#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vulkyrie.h>
#include <vector>
#include <string>

using namespace Vulkyrie;

// ===========================================================================================
// Mock Resource Backend for Testing
// ===========================================================================================

struct MockTextureDescriptor {
    u32 width = 0;
    u32 height = 0;
    std::string format;
};

struct MockTexture {
    using Descriptor = MockTextureDescriptor;

    bool created = false;
    bool destroyed = false;
    i32 preReadCount = 0;
    i32 preWriteCount = 0;

    void Create(const Descriptor &desc, void *allocator) {
        created = true;
        if (allocator) {
            auto *stats = static_cast<std::vector<std::string> *>(allocator);
            stats->push_back("Create:" + std::to_string(desc.width) + "x" + std::to_string(desc.height));
        }
    }

    void Destroy(const Descriptor &desc, void *allocator) {
        destroyed = true;
        if (allocator) {
            auto *stats = static_cast<std::vector<std::string> *>(allocator);
            stats->push_back("Destroy:" + std::to_string(desc.width) + "x" + std::to_string(desc.height));
        }
    }

    void PreRead(i32 flags, void *context) {
        preReadCount++;
        if (context) {
            auto *stats = static_cast<std::vector<std::string> *>(context);
            stats->push_back("PreRead:flags=" + std::to_string(flags));
        }
    }

    void PreWrite(i32 flags, void *context) {
        preWriteCount++;
        if (context) {
            auto *stats = static_cast<std::vector<std::string> *>(context);
            stats->push_back("PreWrite:flags=" + std::to_string(flags));
        }
    }
};

struct MockBuffer {
    using Descriptor = size_t; // Size in bytes

    bool created = false;
    bool destroyed = false;

    void Create(const Descriptor &desc, void *allocator) {
        created = true;
    }

    void Destroy(const Descriptor &desc, void *allocator) {
        destroyed = true;
    }
};

// ===========================================================================================
// Test Cases
// ===========================================================================================

TEST_CASE("FrameGraph - Basic pass addition", "[framegraph]") {
    FrameGraph graph;
    i32 executionCount = 0;

    struct PassData {
        i32 value = 42;
    };

    const auto &data = graph.AddPass<PassData>(
        "TestPass",
        [](FrameGraph::Builder &builder, PassData &data) { builder.SetSideEffect(); },
        [&executionCount](const PassData &data, void *context) {
            executionCount++;
            REQUIRE(data.value == 42);
        });

    REQUIRE(data.value == 42);

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - Resource creation and access", "[framegraph]") {
    FrameGraph graph;
    bool passExecuted = false;

    struct PassData {
        ResourceID texture;
    };

    graph.AddPass<PassData>(
        "CreateTexture",
        [](FrameGraph::Builder &builder, PassData &data) {
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.texture = builder.Create<MockTexture>("MainTexture", desc);
            builder.SetSideEffect();
        },
        [&passExecuted](const PassData &data, void *context) { passExecuted = true; });

    graph.Compile();

    std::vector<std::string> stats;
    graph.Execute(nullptr, &stats);

    REQUIRE(passExecuted);
    REQUIRE(stats.size() >= 1);
    REQUIRE(stats[0] == "Create:1920x1080");
}

TEST_CASE("FrameGraph - Read and Write operations", "[framegraph]") {
    FrameGraph graph;
    ResourceID sharedTexture;

    struct ProducerData {
        ResourceID output;
    };

    struct ConsumerData {
        ResourceID input;
    };

    // Producer pass creates and writes to texture
    graph.AddPass<ProducerData>(
        "Producer",
        [&sharedTexture](FrameGraph::Builder &builder, ProducerData &data) {
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("SharedTexture", desc);
            sharedTexture = data.output;
        },
        [](const ProducerData &data, void *context) {
            // Write to texture
        });

    // Consumer pass reads from texture
    graph.AddPass<ConsumerData>(
        "Consumer",
        [&sharedTexture](FrameGraph::Builder &builder, ConsumerData &data) {
            data.input = builder.Read(sharedTexture);
            builder.SetSideEffect();
        },
        [](const ConsumerData &data, void *context) {
            // Read from texture
        });

    graph.Compile();

    std::vector<std::string> stats;
    graph.Execute(nullptr, &stats);

    REQUIRE(stats.size() >= 2); // Create and Destroy
    REQUIRE(stats[0] == "Create:512x512");
}

TEST_CASE("FrameGraph - Pass culling for unreferenced resources", "[framegraph]") {
    FrameGraph graph;
    i32 producerExecuted = 0;
    i32 consumerExecuted = 0;

    struct ProducerData {
        ResourceID texture;
    };

    struct ConsumerData {};

    // Producer without consumers and no side effects - should be culled
    graph.AddPass<ProducerData>(
        "UnusedProducer",
        [](FrameGraph::Builder &builder, ProducerData &data) {
            MockTextureDescriptor desc{ 256, 256, "RGBA8" };
            data.texture = builder.Create<MockTexture>("UnusedTexture", desc);
            // No side effects, so this pass should be culled
        },
        [&producerExecuted](const ProducerData &data, void *context) { producerExecuted++; });

    // Consumer with side effects - should always execute
    graph.AddPass<ConsumerData>(
        "ConsumerWithSideEffects",
        [](FrameGraph::Builder &builder, ConsumerData &data) { builder.SetSideEffect(); },
        [&consumerExecuted](const ConsumerData &data, void *context) { consumerExecuted++; });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(producerExecuted == 0); // Should be culled
    REQUIRE(consumerExecuted == 1); // Should execute
}

TEST_CASE("FrameGraph - Side effects prevent culling", "[framegraph]") {
    FrameGraph graph;
    i32 executionCount = 0;

    struct PassData {
        ResourceID texture;
    };

    // Pass with side effects should not be culled even without consumers
    graph.AddPass<PassData>(
        "PassWithSideEffects",
        [](FrameGraph::Builder &builder, PassData &data) {
            MockTextureDescriptor desc{ 128, 128, "RGBA8" };
            data.texture = builder.Create<MockTexture>("SideEffectTexture", desc);
            builder.SetSideEffect();
        },
        [&executionCount](const PassData &data, void *context) { executionCount++; });

    graph.Compile();

    std::vector<std::string> stats;
    graph.Execute(nullptr, &stats);

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - Multiple passes with dependencies", "[framegraph]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;
    ResourceID texture1, texture2;

    struct Pass1Data {
        ResourceID output;
    };

    struct Pass2Data {
        ResourceID input;
        ResourceID output;
    };

    struct Pass3Data {
        ResourceID input;
    };

    // Pass 1: Create texture
    graph.AddPass<Pass1Data>(
        "Pass1",
        [&texture1](FrameGraph::Builder &builder, Pass1Data &data) {
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("Texture1", desc);
            texture1 = data.output;
        },
        [&executionOrder](const Pass1Data &data, void *context) { executionOrder.push_back("Pass1"); });

    // Pass 2: Read texture1, create texture2
    graph.AddPass<Pass2Data>(
        "Pass2",
        [&texture1, &texture2](FrameGraph::Builder &builder, Pass2Data &data) {
            data.input = builder.Read(texture1);
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("Texture2", desc);
            texture2 = data.output;
        },
        [&executionOrder](const Pass2Data &data, void *context) { executionOrder.push_back("Pass2"); });

    // Pass 3: Read texture2
    graph.AddPass<Pass3Data>(
        "Pass3",
        [&texture2](FrameGraph::Builder &builder, Pass3Data &data) {
            data.input = builder.Read(texture2);
            builder.SetSideEffect();
        },
        [&executionOrder](const Pass3Data &data, void *context) { executionOrder.push_back("Pass3"); });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executionOrder.size() == 3);
    REQUIRE(executionOrder[0] == "Pass1");
    REQUIRE(executionOrder[1] == "Pass2");
    REQUIRE(executionOrder[2] == "Pass3");
}

TEST_CASE("FrameGraph - Resource Write creates new version", "[framegraph]") {
    FrameGraph graph;
    ResourceID texture1, texture2;
    i32 pass1Executed = 0;
    i32 pass2Executed = 0;
    i32 pass3Executed = 0;

    struct Pass1Data {
        ResourceID output;
    };

    struct Pass2Data {
        ResourceID modified;
    };

    struct Pass3Data {
        ResourceID input;
    };

    // Pass 1: Create texture
    graph.AddPass<Pass1Data>(
        "CreatePass",
        [&texture1](FrameGraph::Builder &builder, Pass1Data &data) {
            MockTextureDescriptor desc{ 256, 256, "RGBA8" };
            data.output = builder.Create<MockTexture>("ModifiableTexture", desc);
            texture1 = data.output;
        },
        [&pass1Executed](const Pass1Data &data, void *context) { pass1Executed++; });

    // Pass 2: Write to texture (creates new version)
    graph.AddPass<Pass2Data>(
        "ModifyPass",
        [&texture1, &texture2](FrameGraph::Builder &builder, Pass2Data &data) {
            data.modified = builder.Write(texture1);
            texture2 = data.modified;
        },
        [&pass2Executed](const Pass2Data &data, void *context) { pass2Executed++; });

    // Pass 3: Read modified texture
    graph.AddPass<Pass3Data>(
        "ReadPass",
        [&texture2](FrameGraph::Builder &builder, Pass3Data &data) {
            data.input = builder.Read(texture2);
            builder.SetSideEffect();
        },
        [&pass3Executed](const Pass3Data &data, void *context) { pass3Executed++; });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(pass1Executed == 1);
    REQUIRE(pass2Executed == 1);
    REQUIRE(pass3Executed == 1);
}

TEST_CASE("FrameGraph - PreRead and PreWrite with flags", "[framegraph]") {
    FrameGraph graph;
    ResourceID texture;

    struct PassData {
        ResourceID res;
    };

    constexpr i32 READ_FLAG = 0x01;
    constexpr i32 WRITE_FLAG = 0x02;

    // Create texture
    graph.AddPass<PassData>(
        "CreatePass",
        [&texture](FrameGraph::Builder &builder, PassData &data) {
            MockTextureDescriptor desc{ 128, 128, "RGBA8" };
            data.res = builder.Create<MockTexture>("TextureWithFlags", desc);
            texture = data.res;
        },
        [](const PassData &data, void *context) {});

    // Read with flags
    graph.AddPass<PassData>(
        "ReadPass",
        [&texture](FrameGraph::Builder &builder, PassData &data) {
            data.res = builder.Read(texture, READ_FLAG);
            builder.SetSideEffect();
        },
        [](const PassData &data, void *context) {});

    graph.Compile();

    std::vector<std::string> stats;
    graph.Execute(nullptr, &stats);

    bool foundPreRead = false;
    for (const auto &stat : stats) {
        if (stat.find("PreRead:flags=1") != std::string::npos) {
            foundPreRead = true;
        }
    }
    REQUIRE(foundPreRead);
}

TEST_CASE("FrameGraph - Diamond dependency pattern", "[framegraph]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;
    ResourceID source, intermediate1, intermediate2;

    struct SourceData {
        ResourceID output;
    };

    struct Path1Data {
        ResourceID input;
        ResourceID output;
    };

    struct Path2Data {
        ResourceID input;
        ResourceID output;
    };

    struct MergeData {
        ResourceID input1;
        ResourceID input2;
    };

    // Source pass
    graph.AddPass<SourceData>(
        "Source",
        [&source](FrameGraph::Builder &builder, SourceData &data) {
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("Source", desc);
            source = data.output;
        },
        [&executionOrder](const SourceData &data, void *context) { executionOrder.push_back("Source"); });

    // Path 1
    graph.AddPass<Path1Data>(
        "Path1",
        [&source, &intermediate1](FrameGraph::Builder &builder, Path1Data &data) {
            data.input = builder.Read(source);
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("Intermediate1", desc);
            intermediate1 = data.output;
        },
        [&executionOrder](const Path1Data &data, void *context) { executionOrder.push_back("Path1"); });

    // Path 2
    graph.AddPass<Path2Data>(
        "Path2",
        [&source, &intermediate2](FrameGraph::Builder &builder, Path2Data &data) {
            data.input = builder.Read(source);
            MockTextureDescriptor desc{ 512, 512, "RGBA8" };
            data.output = builder.Create<MockTexture>("Intermediate2", desc);
            intermediate2 = data.output;
        },
        [&executionOrder](const Path2Data &data, void *context) { executionOrder.push_back("Path2"); });

    // Merge pass
    graph.AddPass<MergeData>(
        "Merge",
        [&intermediate1, &intermediate2](FrameGraph::Builder &builder, MergeData &data) {
            data.input1 = builder.Read(intermediate1);
            data.input2 = builder.Read(intermediate2);
            builder.SetSideEffect();
        },
        [&executionOrder](const MergeData &data, void *context) { executionOrder.push_back("Merge"); });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executionOrder.size() == 4);
    REQUIRE(executionOrder[0] == "Source");
    // Path1 and Path2 can execute in any order
    REQUIRE(executionOrder[3] == "Merge"); // Merge must be last
}

TEST_CASE("FrameGraph - Resource lifetime tracking", "[framegraph]") {
    FrameGraph graph;
    ResourceID texture;
    std::vector<std::string> lifecycle;

    struct Pass1Data {
        ResourceID output;
    };

    struct Pass2Data {
        ResourceID input;
    };

    // Create resource
    graph.AddPass<Pass1Data>(
        "CreateResource",
        [&texture](FrameGraph::Builder &builder, Pass1Data &data) {
            MockTextureDescriptor desc{ 256, 256, "RGBA8" };
            data.output = builder.Create<MockTexture>("LifetimeTest", desc);
            texture = data.output;
        },
        [](const Pass1Data &data, void *context) {});

    // Use resource (last usage)
    graph.AddPass<Pass2Data>(
        "UseResource",
        [&texture](FrameGraph::Builder &builder, Pass2Data &data) {
            data.input = builder.Read(texture);
            builder.SetSideEffect();
        },
        [](const Pass2Data &data, void *context) {});

    graph.Compile();

    std::vector<std::string> stats;
    graph.Execute(nullptr, &stats);

    // Resource should be created and then destroyed after last use
    i32 createIndex = -1;
    i32 destroyIndex = -1;

    for (size_t i = 0; i < stats.size(); ++i) {
        if (stats[i].find("Create:") != std::string::npos) {
            createIndex = static_cast<i32>(i);
        }
        if (stats[i].find("Destroy:") != std::string::npos) {
            destroyIndex = static_cast<i32>(i);
        }
    }

    REQUIRE(createIndex != -1);
    REQUIRE(destroyIndex != -1);
    REQUIRE(createIndex < destroyIndex);
}

TEST_CASE("FrameGraph - Multiple resource types", "[framegraph]") {
    FrameGraph graph;
    i32 executionCount = 0;

    struct PassData {
        ResourceID texture;
        ResourceID buffer;
    };

    graph.AddPass<PassData>(
        "MultiResourcePass",
        [](FrameGraph::Builder &builder, PassData &data) {
            MockTextureDescriptor texDesc{ 512, 512, "RGBA8" };
            data.texture = builder.Create<MockTexture>("MultiTexture", texDesc);

            size_t bufferSize = 1024;
            data.buffer = builder.Create<MockBuffer>("MultiBuffer", bufferSize);

            builder.SetSideEffect();
        },
        [&executionCount](const PassData &data, void *context) { executionCount++; });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - Complex chain with multiple reads and writes", "[framegraph]") {
    FrameGraph graph;
    std::vector<i32> executionOrder;
    ResourceID res;

    struct CreateData {
        ResourceID output;
    };

    struct ModifyData {
        ResourceID inOut;
    };

    struct ReadData {
        ResourceID input;
    };

    // Create
    graph.AddPass<CreateData>(
        "Create",
        [&res](FrameGraph::Builder &builder, CreateData &data) {
            MockTextureDescriptor desc{ 256, 256, "RGBA8" };
            data.output = builder.Create<MockTexture>("ChainResource", desc);
            res = data.output;
        },
        [&executionOrder](const CreateData &data, void *context) { executionOrder.push_back(1); });

    // Modify 1
    graph.AddPass<ModifyData>(
        "Modify1",
        [&res](FrameGraph::Builder &builder, ModifyData &data) {
            data.inOut = builder.Write(res);
            res = data.inOut;
        },
        [&executionOrder](const ModifyData &data, void *context) { executionOrder.push_back(2); });

    // Modify 2
    graph.AddPass<ModifyData>(
        "Modify2",
        [&res](FrameGraph::Builder &builder, ModifyData &data) {
            data.inOut = builder.Write(res);
            res = data.inOut;
        },
        [&executionOrder](const ModifyData &data, void *context) { executionOrder.push_back(3); });

    // Read
    graph.AddPass<ReadData>(
        "Read",
        [&res](FrameGraph::Builder &builder, ReadData &data) {
            data.input = builder.Read(res);
            builder.SetSideEffect();
        },
        [&executionOrder](const ReadData &data, void *context) { executionOrder.push_back(4); });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executionOrder.size() == 4);
    REQUIRE(executionOrder[0] == 1);
    REQUIRE(executionOrder[1] == 2);
    REQUIRE(executionOrder[2] == 3);
    REQUIRE(executionOrder[3] == 4);
}

TEST_CASE("FrameGraphBlackboard - Store and retrieve data", "[framegraph][blackboard]") {
    FrameGraphBlackboard blackboard;

    struct TestData {
        i32 value = 42;
        std::string name = "test";
    };

    SECTION("Set and Get") {
        auto &data = blackboard.Set<TestData>();
        REQUIRE(data.value == 42);
        REQUIRE(data.name == "test");

        auto &retrieved = blackboard.Get<TestData>();
        REQUIRE(retrieved.value == 42);
        REQUIRE(retrieved.name == "test");
    }

    SECTION("Contains check") {
        REQUIRE_FALSE(blackboard.Contains<TestData>());
        blackboard.Set<TestData>();
        REQUIRE(blackboard.Contains<TestData>());
    }

    SECTION("TryGet with missing data") {
        auto *ptr = blackboard.TryGet<TestData>();
        REQUIRE(ptr == nullptr);

        blackboard.Set<TestData>();
        ptr = blackboard.TryGet<TestData>();
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr->value == 42);
    }
}

TEST_CASE("FrameGraphBlackboard - Multiple types", "[framegraph][blackboard]") {
    FrameGraphBlackboard blackboard;

    struct TypeA {
        i32 x = 10;
    };

    struct TypeB {
        f32 y = 3.14f;
    };

    blackboard.Set<TypeA>();
    blackboard.Set<TypeB>();

    REQUIRE(blackboard.Contains<TypeA>());
    REQUIRE(blackboard.Contains<TypeB>());

    REQUIRE(blackboard.Get<TypeA>().x == 10);
    REQUIRE(blackboard.Get<TypeB>().y == 3.14f);
}

TEST_CASE("FrameGraph - Empty graph", "[framegraph]") {
    FrameGraph graph;

    // Should not crash
    graph.Compile();
    graph.Execute(nullptr, nullptr);

    SUCCEED();
}

TEST_CASE("FrameGraph - All passes culled except side effects", "[framegraph]") {
    FrameGraph graph;
    i32 executed = 0;

    struct PassData {
        ResourceID res;
    };

    // Add 5 passes, only one with side effects
    for (i32 i = 0; i < 4; ++i) {
        graph.AddPass<PassData>(
            "CulledPass",
            [](FrameGraph::Builder &builder, PassData &data) {
                MockTextureDescriptor desc{ 128, 128, "RGBA8" };
                data.res = builder.Create<MockTexture>("CulledRes", desc);
                // No side effects
            },
            [&executed](const PassData &data, void *context) { executed++; });
    }

    graph.AddPass<PassData>(
        "KeptPass",
        [](FrameGraph::Builder &builder, PassData &data) {
            MockTextureDescriptor desc{ 128, 128, "RGBA8" };
            data.res = builder.Create<MockTexture>("KeptRes", desc);
            builder.SetSideEffect();
        },
        [&executed](const PassData &data, void *context) { executed++; });

    graph.Compile();
    graph.Execute(nullptr, nullptr);

    REQUIRE(executed == 1); // Only the pass with side effects should execute
}

TEST_CASE("FrameGraph - Complex deferred rendering pipeline", "[framegraph]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;

    // Configuration flags to control which passes execute
    bool enableSpotLight2 = false; // Culled light
    bool enablePointLight = false; // Culled light
    bool enableSSAO = true;
    bool enableBloom = true;
    bool enableDebugVisualization = false; // Debug passes will be culled

    // Resource handles for the pipeline
    ResourceID dirShadowMap, spot1ShadowMap, spot2ShadowMap, pointShadowMap;
    ResourceID gbufferAlbedo, gbufferNormal, gbufferDepth, gbufferMaterial;
    ResourceID ssaoTexture, ssaoBlurred;
    ResourceID sceneColor, sceneDepth;
    ResourceID bloomDown1, bloomDown2, bloomDown3;
    ResourceID bloomUp1, bloomUp2;
    ResourceID toneMappedColor, gradedColor, finalColor;
    ResourceID debugOutput;

    // Shadow map passes
    struct ShadowMapData {
        ResourceID shadowMap;
    };

    graph.AddPass<ShadowMapData>(
        "DirectionalShadowMap",
        [&dirShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            MockTextureDescriptor desc{ 2048, 2048, "D32" };
            data.shadowMap = builder.Create<MockTexture>("DirShadowMap", desc);
            dirShadowMap = data.shadowMap;
        },
        [&executionOrder](const ShadowMapData &data, void *context) { executionOrder.push_back("DirectionalShadowMap"); });

    graph.AddPass<ShadowMapData>(
        "SpotLight1ShadowMap",
        [&spot1ShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            MockTextureDescriptor desc{ 1024, 1024, "D32" };
            data.shadowMap = builder.Create<MockTexture>("Spot1ShadowMap", desc);
            spot1ShadowMap = data.shadowMap;
        },
        [&executionOrder](const ShadowMapData &data, void *context) { executionOrder.push_back("SpotLight1ShadowMap"); });

    // This shadow map will be culled (not used by lighting pass)
    graph.AddPass<ShadowMapData>(
        "SpotLight2ShadowMap",
        [&spot2ShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            MockTextureDescriptor desc{ 1024, 1024, "D32" };
            data.shadowMap = builder.Create<MockTexture>("Spot2ShadowMap", desc);
            spot2ShadowMap = data.shadowMap;
        },
        [&executionOrder](const ShadowMapData &data, void *context) { executionOrder.push_back("SpotLight2ShadowMap"); });

    // This shadow map will also be culled
    graph.AddPass<ShadowMapData>(
        "PointLightShadowMap",
        [&pointShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            MockTextureDescriptor desc{ 512, 512, "D32" };
            data.shadowMap = builder.Create<MockTexture>("PointShadowMap", desc);
            pointShadowMap = data.shadowMap;
        },
        [&executionOrder](const ShadowMapData &data, void *context) { executionOrder.push_back("PointLightShadowMap"); });

    // G-Buffer pass
    struct GBufferData {
        ResourceID albedo, normal, depth, material;
    };

    graph.AddPass<GBufferData>(
        "GBufferPass",
        [&gbufferAlbedo, &gbufferNormal, &gbufferDepth, &gbufferMaterial](FrameGraph::Builder &builder, GBufferData &data) {
            MockTextureDescriptor albedoDesc{ 1920, 1080, "RGBA8" };
            MockTextureDescriptor normalDesc{ 1920, 1080, "RGBA16F" };
            MockTextureDescriptor depthDesc{ 1920, 1080, "D32" };
            MockTextureDescriptor materialDesc{ 1920, 1080, "RGBA8" };

            data.albedo = builder.Create<MockTexture>("GBufferAlbedo", albedoDesc);
            data.normal = builder.Create<MockTexture>("GBufferNormal", normalDesc);
            data.depth = builder.Create<MockTexture>("GBufferDepth", depthDesc);
            data.material = builder.Create<MockTexture>("GBufferMaterial", materialDesc);

            gbufferAlbedo = data.albedo;
            gbufferNormal = data.normal;
            gbufferDepth = data.depth;
            gbufferMaterial = data.material;
        },
        [&executionOrder](const GBufferData &data, void *context) { executionOrder.push_back("GBufferPass"); });

    // SSAO Pass
    struct SSAOData {
        ResourceID depthIn, normalIn, ssaoOut;
    };

    if (enableSSAO) {
        graph.AddPass<SSAOData>(
            "SSAOPass",
            [&gbufferDepth, &gbufferNormal, &ssaoTexture](FrameGraph::Builder &builder, SSAOData &data) {
                data.depthIn = builder.Read(gbufferDepth);
                data.normalIn = builder.Read(gbufferNormal);
                MockTextureDescriptor desc{ 1920, 1080, "R8" };
                data.ssaoOut = builder.Create<MockTexture>("SSAO", desc);
                ssaoTexture = data.ssaoOut;
            },
            [&executionOrder](const SSAOData &data, void *context) { executionOrder.push_back("SSAOPass"); });

        // SSAO Blur
        struct SSAOBlurData {
            ResourceID ssaoIn, ssaoOut;
        };

        graph.AddPass<SSAOBlurData>(
            "SSAOBlurPass",
            [&ssaoTexture, &ssaoBlurred](FrameGraph::Builder &builder, SSAOBlurData &data) {
                data.ssaoIn = builder.Read(ssaoTexture);
                MockTextureDescriptor desc{ 1920, 1080, "R8" };
                data.ssaoOut = builder.Create<MockTexture>("SSAOBlurred", desc);
                ssaoBlurred = data.ssaoOut;
            },
            [&executionOrder](const SSAOBlurData &data, void *context) { executionOrder.push_back("SSAOBlurPass"); });
    }

    // Lighting Pass
    struct LightingData {
        ResourceID albedoIn, normalIn, depthIn, materialIn;
        ResourceID dirShadowIn, spot1ShadowIn;
        ResourceID ssaoIn;
        ResourceID colorOut, depthOut;
    };

    graph.AddPass<LightingData>(
        "LightingPass",
        [&](FrameGraph::Builder &builder, LightingData &data) {
            data.albedoIn = builder.Read(gbufferAlbedo);
            data.normalIn = builder.Read(gbufferNormal);
            data.depthIn = builder.Read(gbufferDepth);
            data.materialIn = builder.Read(gbufferMaterial);
            data.dirShadowIn = builder.Read(dirShadowMap);
            data.spot1ShadowIn = builder.Read(spot1ShadowMap);
            // Only reading active shadow maps - spot2 and point will be culled

            if (enableSSAO) {
                data.ssaoIn = builder.Read(ssaoBlurred);
            }

            MockTextureDescriptor colorDesc{ 1920, 1080, "RGBA16F" };
            MockTextureDescriptor depthDesc{ 1920, 1080, "D32" };
            data.colorOut = builder.Create<MockTexture>("SceneColor", colorDesc);
            data.depthOut = builder.Create<MockTexture>("SceneDepth", depthDesc);

            sceneColor = data.colorOut;
            sceneDepth = data.depthOut;
        },
        [&executionOrder](const LightingData &data, void *context) { executionOrder.push_back("LightingPass"); });

    // Sky Pass
    struct SkyData {
        ResourceID depthIn, colorInOut;
    };

    graph.AddPass<SkyData>(
        "SkyPass",
        [&sceneDepth, &sceneColor](FrameGraph::Builder &builder, SkyData &data) {
            data.depthIn = builder.Read(sceneDepth);
            data.colorInOut = builder.Write(sceneColor);
            sceneColor = data.colorInOut;
        },
        [&executionOrder](const SkyData &data, void *context) { executionOrder.push_back("SkyPass"); });

    // Transparent Pass
    struct TransparentData {
        ResourceID colorInOut, depthIn;
    };

    graph.AddPass<TransparentData>(
        "TransparentPass",
        [&sceneColor, &sceneDepth](FrameGraph::Builder &builder, TransparentData &data) {
            data.depthIn = builder.Read(sceneDepth);
            data.colorInOut = builder.Write(sceneColor);
            sceneColor = data.colorInOut;
        },
        [&executionOrder](const TransparentData &data, void *context) { executionOrder.push_back("TransparentPass"); });

    // Bloom chain (if enabled)
    if (enableBloom) {
        struct BloomDownsampleData {
            ResourceID input, output;
        };

        graph.AddPass<BloomDownsampleData>(
            "BloomDownsample1",
            [&sceneColor, &bloomDown1](FrameGraph::Builder &builder, BloomDownsampleData &data) {
                data.input = builder.Read(sceneColor);
                MockTextureDescriptor desc{ 960, 540, "RGBA16F" };
                data.output = builder.Create<MockTexture>("BloomDown1", desc);
                bloomDown1 = data.output;
            },
            [&executionOrder](const BloomDownsampleData &data, void *context) { executionOrder.push_back("BloomDownsample1"); });

        graph.AddPass<BloomDownsampleData>(
            "BloomDownsample2",
            [&bloomDown1, &bloomDown2](FrameGraph::Builder &builder, BloomDownsampleData &data) {
                data.input = builder.Read(bloomDown1);
                MockTextureDescriptor desc{ 480, 270, "RGBA16F" };
                data.output = builder.Create<MockTexture>("BloomDown2", desc);
                bloomDown2 = data.output;
            },
            [&executionOrder](const BloomDownsampleData &data, void *context) { executionOrder.push_back("BloomDownsample2"); });

        graph.AddPass<BloomDownsampleData>(
            "BloomDownsample3",
            [&bloomDown2, &bloomDown3](FrameGraph::Builder &builder, BloomDownsampleData &data) {
                data.input = builder.Read(bloomDown2);
                MockTextureDescriptor desc{ 240, 135, "RGBA16F" };
                data.output = builder.Create<MockTexture>("BloomDown3", desc);
                bloomDown3 = data.output;
            },
            [&executionOrder](const BloomDownsampleData &data, void *context) { executionOrder.push_back("BloomDownsample3"); });

        struct BloomUpsampleData {
            ResourceID input, output;
        };

        graph.AddPass<BloomUpsampleData>(
            "BloomUpsample1",
            [&bloomDown3, &bloomUp1](FrameGraph::Builder &builder, BloomUpsampleData &data) {
                data.input = builder.Read(bloomDown3);
                MockTextureDescriptor desc{ 480, 270, "RGBA16F" };
                data.output = builder.Create<MockTexture>("BloomUp1", desc);
                bloomUp1 = data.output;
            },
            [&executionOrder](const BloomUpsampleData &data, void *context) { executionOrder.push_back("BloomUpsample1"); });

        graph.AddPass<BloomUpsampleData>(
            "BloomUpsample2",
            [&bloomUp1, &bloomUp2](FrameGraph::Builder &builder, BloomUpsampleData &data) {
                data.input = builder.Read(bloomUp1);
                MockTextureDescriptor desc{ 960, 540, "RGBA16F" };
                data.output = builder.Create<MockTexture>("BloomUp2", desc);
                bloomUp2 = data.output;
            },
            [&executionOrder](const BloomUpsampleData &data, void *context) { executionOrder.push_back("BloomUpsample2"); });

        // Bloom combine
        struct BloomCombineData {
            ResourceID scene, bloom, output;
        };

        graph.AddPass<BloomCombineData>(
            "BloomCombine",
            [&sceneColor, &bloomUp2](FrameGraph::Builder &builder, BloomCombineData &data) {
                data.scene = builder.Read(sceneColor);
                data.bloom = builder.Read(bloomUp2);
                data.output = builder.Write(sceneColor);
                sceneColor = data.output;
            },
            [&executionOrder](const BloomCombineData &data, void *context) { executionOrder.push_back("BloomCombine"); });
    }

    // Tone Mapping
    struct ToneMappingData {
        ResourceID input, output;
    };

    graph.AddPass<ToneMappingData>(
        "ToneMappingPass",
        [&sceneColor, &toneMappedColor](FrameGraph::Builder &builder, ToneMappingData &data) {
            data.input = builder.Read(sceneColor);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("ToneMapped", desc);
            toneMappedColor = data.output;
        },
        [&executionOrder](const ToneMappingData &data, void *context) { executionOrder.push_back("ToneMappingPass"); });

    // Color Grading
    struct ColorGradingData {
        ResourceID input, output;
    };

    graph.AddPass<ColorGradingData>(
        "ColorGradingPass",
        [&toneMappedColor, &gradedColor](FrameGraph::Builder &builder, ColorGradingData &data) {
            data.input = builder.Read(toneMappedColor);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("ColorGraded", desc);
            gradedColor = data.output;
        },
        [&executionOrder](const ColorGradingData &data, void *context) { executionOrder.push_back("ColorGradingPass"); });

    // FXAA
    struct FXAAData {
        ResourceID input, output;
    };

    graph.AddPass<FXAAData>(
        "FXAAPass",
        [&gradedColor, &finalColor](FrameGraph::Builder &builder, FXAAData &data) {
            data.input = builder.Read(gradedColor);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("FinalColor", desc);
            finalColor = data.output;
        },
        [&executionOrder](const FXAAData &data, void *context) { executionOrder.push_back("FXAAPass"); });

    // Debug passes (will be culled if not used)
    struct DebugData {
        ResourceID input, output;
    };

    graph.AddPass<DebugData>(
        "DebugWireframePass",
        [&gbufferDepth, &debugOutput](FrameGraph::Builder &builder, DebugData &data) {
            data.input = builder.Read(gbufferDepth);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("DebugWireframe", desc);
            debugOutput = data.output;
        },
        [&executionOrder](const DebugData &data, void *context) { executionOrder.push_back("DebugWireframePass"); });

    graph.AddPass<DebugData>(
        "DebugNormalsPass",
        [&gbufferNormal](FrameGraph::Builder &builder, DebugData &data) {
            data.input = builder.Read(gbufferNormal);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("DebugNormals", desc);
        },
        [&executionOrder](const DebugData &data, void *context) { executionOrder.push_back("DebugNormalsPass"); });

    graph.AddPass<DebugData>(
        "DebugLightHeatmapPass",
        [&gbufferAlbedo](FrameGraph::Builder &builder, DebugData &data) {
            data.input = builder.Read(gbufferAlbedo);
            MockTextureDescriptor desc{ 1920, 1080, "RGBA8" };
            data.output = builder.Create<MockTexture>("DebugHeatmap", desc);
        },
        [&executionOrder](const DebugData &data, void *context) { executionOrder.push_back("DebugLightHeatmapPass"); });

    // UI Pass
    struct UIData {
        ResourceID colorInOut;
    };

    graph.AddPass<UIData>(
        "UIPass",
        [&finalColor](FrameGraph::Builder &builder, UIData &data) {
            data.colorInOut = builder.Write(finalColor);
            finalColor = data.colorInOut;
        },
        [&executionOrder](const UIData &data, void *context) { executionOrder.push_back("UIPass"); });

    // Final present pass (always executes - has side effect)
    struct PresentData {
        ResourceID finalImage;
    };

    graph.AddPass<PresentData>(
        "PresentPass",
        [&finalColor](FrameGraph::Builder &builder, PresentData &data) {
            data.finalImage = builder.Read(finalColor);
            builder.SetSideEffect(); // Present to screen
        },
        [&executionOrder](const PresentData &data, void *context) { executionOrder.push_back("PresentPass"); });

    // Compile and execute
    graph.Compile();
    graph.Execute(nullptr, nullptr);

    // Verify execution order and culling
    REQUIRE(executionOrder.size() >= 15); // At least the main pipeline passes

    // These passes should always execute (part of critical path to present)
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "DirectionalShadowMap") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "SpotLight1ShadowMap") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "GBufferPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "LightingPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "SkyPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "TransparentPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "ToneMappingPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "ColorGradingPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "FXAAPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "UIPass") != executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "PresentPass") != executionOrder.end());

    // These passes should be culled (not referenced by lighting pass)
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "SpotLight2ShadowMap") == executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "PointLightShadowMap") == executionOrder.end());

    // Debug passes should be culled (no side effects and output not used)
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "DebugWireframePass") == executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "DebugNormalsPass") == executionOrder.end());
    REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "DebugLightHeatmapPass") == executionOrder.end());

    // SSAO passes should execute if enabled
    if (enableSSAO) {
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "SSAOPass") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "SSAOBlurPass") != executionOrder.end());
    }

    // Bloom passes should execute if enabled
    if (enableBloom) {
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomDownsample1") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomDownsample2") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomDownsample3") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomUpsample1") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomUpsample2") != executionOrder.end());
        REQUIRE(std::find(executionOrder.begin(), executionOrder.end(), "BloomCombine") != executionOrder.end());
    }

    // Verify proper ordering: GBuffer before Lighting, Lighting before ToneMapping, etc.
    auto gbufferPos = std::find(executionOrder.begin(), executionOrder.end(), "GBufferPass");
    auto lightingPos = std::find(executionOrder.begin(), executionOrder.end(), "LightingPass");
    auto toneMappingPos = std::find(executionOrder.begin(), executionOrder.end(), "ToneMappingPass");
    auto presentPos = std::find(executionOrder.begin(), executionOrder.end(), "PresentPass");

    REQUIRE(gbufferPos < lightingPos);
    REQUIRE(lightingPos < toneMappingPos);
    REQUIRE(toneMappingPos < presentPos);
}
