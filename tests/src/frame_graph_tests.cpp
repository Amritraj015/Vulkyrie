#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vulkyrie.h>
#include <vector>
#include <string>

using namespace Vulkyrie::Renderer;

// ===========================================================================================
// Mock Resource Backend for Testing
// ===========================================================================================

struct MockTextureDescriptor {
        uint32_t width = 0;
        uint32_t height = 0;
        std::string format;
};

struct MockTexture {
        using Descriptor = MockTextureDescriptor;

        bool created = false;
        bool destroyed = false;
        int preReadCount = 0;
        int preWriteCount = 0;

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
    int executionCount = 0;

    struct PassData {
            int value = 42;
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
    int producerExecuted = 0;
    int consumerExecuted = 0;

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
    int executionCount = 0;

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
    int pass1Executed = 0;
    int pass2Executed = 0;
    int pass3Executed = 0;

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
    int createIndex = -1;
    int destroyIndex = -1;

    for (size_t i = 0; i < stats.size(); ++i) {
        if (stats[i].find("Create:") != std::string::npos) {
            createIndex = static_cast<int>(i);
        }
        if (stats[i].find("Destroy:") != std::string::npos) {
            destroyIndex = static_cast<int>(i);
        }
    }

    REQUIRE(createIndex != -1);
    REQUIRE(destroyIndex != -1);
    REQUIRE(createIndex < destroyIndex);
}

TEST_CASE("FrameGraph - Multiple resource types", "[framegraph]") {
    FrameGraph graph;
    int executionCount = 0;

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
    std::vector<int> executionOrder;
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
            int value = 42;
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
            int x = 10;
    };

    struct TypeB {
            float y = 3.14f;
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
    int executed = 0;

    struct PassData {
            ResourceID res;
    };

    // Add 5 passes, only one with side effects
    for (int i = 0; i < 4; ++i) {
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
