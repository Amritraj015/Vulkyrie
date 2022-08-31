#pragma once

#include <Vulkyrie.h>
#include <iostream>

class SandboxApp : public Vkr::Application {
public:
	~SandboxApp() override {
		std::cout << "SandboxApp ending!" << std::endl;
	};

	/* Function pointer to the application's initialize function. */
	bool Initialize() override {
		return true;
	}

	/* Function pointer to the application's update function */
	bool Update(float delta_time) override {
		return true;
	}

	/* Function pointer to the application's render function */
	bool Render(float delta_time) override {
		return true;
	}

	/* Function pointer that handles resizes, if applicable. */
	void OnResize(unsigned short width, unsigned short height) override {
	}
};
