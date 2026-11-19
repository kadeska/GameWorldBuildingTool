// GameWorldBuildingTool.cpp : Defines the entry point for the application.
//

#include "GameWorldBuildingTool.hpp"


void log(const char* prefix, const char* message)
{
	std::cout << "[" << prefix << "] " << message << std::endl;
}

void exitTool() 
{
	log("Main", "Exiting GameWorldBuildingTool");
	// clean up and save any necessary data here
	// 
	// 
	// Wait for user input before exiting
	log("Main", "Press enter to exit...");
	std::cin.get();
}

int main()
{
	std::atexit(exitTool);

	log("Main", "Starting GameWorldBuildingTool");

	openWindow(1280, 720, "Game World Building Tool");

	return 0;
}
