# first_app
Contains the game loop and is the entry point of the application (not including **main()** which right now only constructs first_app and calls it).

# aspen_window
Works with GLFW to setup the window and get it's handle and extent.
Called from **first_app** when it is created.

# aspen_device
Handles the setup of the Vulkan instance and logical device creation.
Is constructed from **first_app** when it is created.

# aspen_renderer
Deals with the creation/recreation of the swap chain, command buffers, and begin and end frames.
Is constructed from **first_app** when it is created after the creation of **aspen_device**.

# aspen_swap_chain
Handles the creation and configuration of the swap chain, image views, render pass, framebuffers, and synchronization objects.
Created from **aspen_renderer** in its constructor.

# {name}_render_system
Handles the rendering of game objects and creating a pipeline layout and passing it into the constructor of **aspen_pipeline** to be used in the creation of the graphics pipeline.
Before constructing **aspen_pipeline**, it calls **aspen_pipeline**'s **defaultPipelineConfigInfo** to setup the default graphics pipeline stage settings.
When constructing **aspen_pipeline**, it passes in the relative paths to the compiled .spv shader files.
Created just outside of the game loop in **first_app**.

# aspen_pipeline
Responsible for creating and configuring the graphics pipeline, and reading compiled .spv shader files.

# aspen_model
Take vertex data created by or read in a file on the CPU and then allocate the memory and copy the data over to our device GPU so it can be rendered efficiently.

# aspen_game_object
Acts as a base class for all game objects. Will later be evolved into a fully functioning ECS (Entity Component System).