// #include "first_app.hpp"

// #include "simple_render_system.hpp"

// // libs
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #include <glm/glm.hpp>
// #include <glm/gtc/constants.hpp>

// // std
// #include <array>
// #include <cassert>
// #include <stdexcept>

// namespace Aspen {
//     class GravityPhysicsSystem {
//     public:
//         GravityPhysicsSystem(float strength) : strengthGravity{strength} {}

//         const float strengthGravity;

//         // dt stands for delta time, and specifies the amount of time to advance the simulation
//         // substeps is how many intervals to divide the forward time step in. More substeps result in a
//         // more stable simulation, but takes longer to compute
//         void update(std::vector<GameObject> &objs, float dt, unsigned int substeps = 1) {
//             const float stepDelta = dt / substeps; // How much time has passed in the current simulation step between each simulation substep?
//             for (int i = 0; i < substeps; i++) {
//                 stepSimulation(objs, stepDelta);
//             }
//         }

//         glm::vec2 computeForce(GameObject &fromObj, GameObject &toObj) const {
//             auto offset = fromObj.transform2d.translation - toObj.transform2d.translation; // Returns a vector.
//             float distanceSquared = glm::dot(offset, offset);                              // Perform dot product with itself in order to get the squared distance.

//             // clown town - just going to return 0 if objects are too close together...
//             if (glm::abs(distanceSquared) < 1e-10f) {
//                 return {.0f, .0f};
//             }

//             // Newton's formula for gravitional force of attraction between two masses.
//             float force =
//                 strengthGravity * toObj.rigidBody2d.mass * fromObj.rigidBody2d.mass / distanceSquared;
//             return force * offset / glm::sqrt(distanceSquared);
//         }

//     private:
//         void stepSimulation(std::vector<GameObject> &physicsObjs, float dt) {
//             // Loops through all pairs of objects and applies attractive force between them
//             for (auto iterA = physicsObjs.begin(); iterA != physicsObjs.end(); ++iterA) {
//                 auto &objA = *iterA;
//                 for (auto iterB = iterA; iterB != physicsObjs.end(); ++iterB) {
//                     if (iterA == iterB)
//                         continue;
//                     auto &objB = *iterB;

//                     auto force = computeForce(objA, objB);

//                     // Calculate the acceleration for each object and add it onto its current velocity.
//                     // F = ma -> a = F/m.
//                     objA.rigidBody2d.velocity += dt * -force / objA.rigidBody2d.mass;
//                     objB.rigidBody2d.velocity += dt * force / objB.rigidBody2d.mass;
//                 }
//             }

//             // update each objects position based on its final velocity
//             for (auto &obj : physicsObjs) {
//                 obj.transform2d.translation += dt * obj.rigidBody2d.velocity;
//             }
//         }
//     };

//     class Vec2FieldSystem {
//     public:
//         void update(
//             const GravityPhysicsSystem &physicsSystem,
//             std::vector<GameObject> &physicsObjs,
//             std::vector<GameObject> &vectorField) {
//             // For each field line we caluclate the net graviation force for that point in space
//             for (auto &vf : vectorField) {
//                 glm::vec2 direction{};
//                 for (auto &obj : physicsObjs) {
//                     direction += physicsSystem.computeForce(obj, vf);
//                 }

//                 // This scales the length of the field line based on the log of the length
//                 // values were chosen just through trial and error based on what i liked the look
//                 // of and then the field line is rotated to point in the direction of the field
//                 vf.transform2d.scale.x =
//                     0.005f + 0.045f * glm::clamp(glm::log(glm::length(direction) + 1) / 3.f, 0.f, 1.f);
//                 vf.transform2d.rotation = atan2(direction.y, direction.x);
//             }
//         }
//     };

//     std::unique_ptr<Model> createSquareModel(Device &device, glm::vec2 offset) {
//         std::vector<Model::Vertex> vertices = {
//             {{-0.5f, -0.5f}},
//             {{0.5f, 0.5f}},
//             {{-0.5f, 0.5f}},
//             {{-0.5f, -0.5f}},
//             {{0.5f, -0.5f}},
//             {{0.5f, 0.5f}},
//         };
//         for (auto &v : vertices) {
//             v.position += offset;
//         }
//         return std::make_unique<Model>(device, vertices);
//     }

//     std::unique_ptr<Model> createCircleModel(Device &device, unsigned int numSides) {
//         std::vector<Model::Vertex> uniqueVertices{};
//         for (int i = 0; i < numSides; i++) {
//             float angle = i * glm::two_pi<float>() / numSides;
//             uniqueVertices.push_back({{glm::cos(angle), glm::sin(angle)}}); // All the vertices on the outside of the circle.
//         }
//         uniqueVertices.push_back({}); // adds center vertex at 0,0

//         std::vector<Model::Vertex> vertices{};
//         for (int i = 0; i < numSides; i++) {
//             vertices.push_back(uniqueVertices[i]);                  // The current vertex.
//             vertices.push_back(uniqueVertices[(i + 1) % numSides]); // The vertex on the diameter next to it.
//             vertices.push_back(uniqueVertices[numSides]);           // The center circle at 0,0
//         }
//         return std::make_unique<Model>(device, vertices);
//     }

//     FirstApp::FirstApp() {}

//     FirstApp::~FirstApp() {}

//     void FirstApp::run() {
//         // create some models
//         std::shared_ptr<Model> squareModel = createSquareModel(
//             device,
//             {.5f, .0f}); // offset model by .5 so rotation occurs at edge rather than center of square
//         std::shared_ptr<Model> circleModel = createCircleModel(device, 64);

//         // create physics objects
//         std::vector<GameObject> physicsObjects{};
//         auto red = GameObject::createGameObject();
//         red.transform2d.scale = glm::vec2{.05f};
//         red.transform2d.translation = {.5f, .5f};
//         red.color = {1.f, 0.f, 0.f};
//         red.rigidBody2d.velocity = {-.5f, .0f};
//         red.model = circleModel;
//         physicsObjects.push_back(std::move(red));
//         auto blue = GameObject::createGameObject();
//         blue.transform2d.scale = glm::vec2{.05f};
//         blue.transform2d.translation = {-.45f, -.25f};
//         blue.color = {0.f, 0.f, 1.f};
//         blue.rigidBody2d.velocity = {.5f, .0f};
//         blue.model = circleModel;
//         physicsObjects.push_back(std::move(blue));

//         // create vector field
//         std::vector<GameObject> vectorField{};
//         int gridCount = 40;
//         for (int i = 0; i < gridCount; i++) {
//             for (int j = 0; j < gridCount; j++) {
//                 auto vf = GameObject::createGameObject();
//                 vf.transform2d.scale = glm::vec2(0.005f);
//                 vf.transform2d.translation = {
//                     -1.0f + (i + 0.5f) * 2.0f / gridCount,
//                     -1.0f + (j + 0.5f) * 2.0f / gridCount};
//                 vf.color = glm::vec3(1.0f);
//                 vf.model = squareModel;
//                 vectorField.push_back(std::move(vf));
//             }
//         }

//         GravityPhysicsSystem gravitySystem{0.81f};
//         Vec2FieldSystem vecFieldSystem{};

//         SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};

//         // Subscribe lambda function to recreate swapchain when resizing window and draw a frame with the updated swapchain. This is done to enable smooth resizing.
//         window.windowResizeSubscribe([this, &simpleRenderSystem, &physicsObjects, &vectorField, &gravitySystem, &vecFieldSystem]() {
//             this->window.resetWindowResizedFlag();
//             this->renderer.recreateSwapChain();
//             // this->createPipeline(); // Right now this is not required as the new render pass will be compatible with the old one but is put here for future proofing.

//             if (auto commandBuffer = this->renderer.beginFrame()) {
//                 // update systems
//                 gravitySystem.update(physicsObjects, 1.f / 250, 5);
//                 vecFieldSystem.update(gravitySystem, physicsObjects, vectorField);

//                 this->renderer.beginSwapChainRenderPass(commandBuffer);
//                 simpleRenderSystem.renderGameObjects(commandBuffer, physicsObjects);
//                 simpleRenderSystem.renderGameObjects(commandBuffer, vectorField);
//                 this->renderer.endSwapChainRenderPass(commandBuffer);
//                 this->renderer.endFrame();
//             }
//         });

//         while (!window.shouldClose()) {
//             glfwPollEvents();

//             if (auto commandBuffer = renderer.beginFrame()) {
//                 // update systems
//                 gravitySystem.update(physicsObjects, 1.f / 250, 5);
//                 vecFieldSystem.update(gravitySystem, physicsObjects, vectorField);

//                 // render system
//                 renderer.beginSwapChainRenderPass(commandBuffer);
//                 simpleRenderSystem.renderGameObjects(commandBuffer, physicsObjects);
//                 simpleRenderSystem.renderGameObjects(commandBuffer, vectorField);
//                 renderer.endSwapChainRenderPass(commandBuffer);
//                 renderer.endFrame();
//             }
//         }

//         vkDeviceWaitIdle(device.device());
//     }
// }
