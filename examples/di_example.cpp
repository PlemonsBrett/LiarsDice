#include "liarsdice/adapters/dice_adapter.hpp"
#include "liarsdice/adapters/random_generator.hpp"
#include "liarsdice/di/simple_container.hpp"
#include "liarsdice/interfaces/interfaces.hpp"
#include <iostream>
#include <vector>

using namespace liarsdice::di;
using namespace liarsdice::interfaces;
using namespace liarsdice::adapters;

// Example service interface and implementation
class IGameLogger {
public:
  virtual ~IGameLogger() = default;
  virtual void log(const std::string &message) = 0;
  virtual void log_dice_roll(int player_id, const std::vector<unsigned int> &values) = 0;
};

class ConsoleLogger : public IGameLogger {
public:
  void log(const std::string &message) override { std::cout << "[LOG] " << message << '\n'; }

  void log_dice_roll(int player_id, const std::vector<unsigned int> &values) override {
    std::cout << "[DICE] Player " << player_id << " rolled: ";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) {
        std::cout << ", ";
      }
      std::cout << values[i];
    }
    std::cout << '\n';
  }
};

// Example service that depends on other services
class DiceRollService {
private:
  std::unique_ptr<IRandomGenerator> rng_;
  std::unique_ptr<IGameLogger> logger_;

public:
  DiceRollService(std::unique_ptr<IRandomGenerator> rng, std::unique_ptr<IGameLogger> logger)
      : rng_(std::move(rng)), logger_(std::move(logger)) {}

  std::vector<unsigned int> roll_dice(int player_id, int dice_count) {
    std::vector<unsigned int> results;
    results.reserve(static_cast<size_t>(dice_count));

    for (int i = 0; i < dice_count; ++i) {
      unsigned int value = static_cast<unsigned int>(rng_->generate(1, 6));
      results.push_back(value);
    }

    logger_->log_dice_roll(player_id, results);
    return results;
  }

  void set_seed(unsigned int seed) {
    rng_->seed(seed);
    logger_->log("Random generator seed set to " + std::to_string(seed));
  }
};

int main() {
  std::cout << "Liar's Dice Dependency Injection Example\n";
  std::cout << "=========================================\n\n";

  try {
    // Create and configure the DI container directly
    auto container = std::make_unique<SimpleContainer>();

    // Register services directly
    std::cout << "Configuring dependency injection container...\n";

    // Register random generator
    container->register_service<IRandomGenerator, StandardRandomGenerator>("game_rng", 12345U);

    // Register logger
    container->register_service<IGameLogger, ConsoleLogger>();

    // Register dice service with factory that resolves dependencies
    auto dice_factory = []() -> std::unique_ptr<DiceRollService> {
      // In a real application, you'd inject the container here
      // For this example, we'll create dependencies manually
      auto rng = std::make_unique<StandardRandomGenerator>(12345U);
      auto logger = std::make_unique<ConsoleLogger>();
      return std::make_unique<DiceRollService>(std::move(rng), std::move(logger));
    };
    container->register_factory<DiceRollService>(dice_factory);
    std::cout << "Container configured with " << container->size() << " services.\n\n";

    // Demonstrate service resolution
    std::cout << "Resolving services and demonstrating functionality...\n\n";

    // Resolve random generator
    auto rng = container->resolve<IRandomGenerator>("game_rng");
    if (!rng) {
      std::cerr << "Failed to resolve random generator!\n";
      return 1;
    }

    std::cout << "Testing random number generation:\n";
    for (int i = 0; i < 5; ++i) {
      int value = rng->generate(1, 6);
      std::cout << "  Random value " << (i + 1) << ": " << value << "\n";
    }
    std::cout << "\n";

    // Resolve logger
    auto logger = container->resolve<IGameLogger>();
    if (!logger) {
      std::cerr << "Failed to resolve logger!\n";
      return 1;
    }

    logger->log("Logger service resolved successfully");
    std::cout << "\n";

    // Resolve dice roll service
    auto dice_service = container->resolve<DiceRollService>();
    if (!dice_service) {
      std::cerr << "Failed to resolve dice roll service!\n";
      return 1;
    }

    // Demonstrate dice rolling
    std::cout << "Simulating dice rolls for multiple players:\n";
    dice_service->set_seed(42); // Set seed for reproducible results

    for (int player = 1; player <= 3; ++player) {
      auto results = dice_service->roll_dice(player, 5); // 5 dice per player
    }

    std::cout << "\nDemonstrating service registration inspection:\n";
    auto registered_services = container->get_registered_services();
    std::cout << "Named services registered: ";
    for (size_t i = 0; i < registered_services.size(); ++i) {
      if (i > 0) {
        std::cout << ", ";
      }
      std::cout << registered_services[i];
    }
    std::cout << "\n\n";

    // Demonstrate concepts validation at compile time
    std::cout << "All services satisfy their interface concepts at compile time.\n";
    std::cout << "This ensures type safety and proper interface implementation.\n\n";

    // Demonstrate multiple resolution of transient services
    std::cout << "Demonstrating transient service behavior:\n";
    auto dice_service2 = container->resolve<DiceRollService>();
    if (dice_service2) {
      std::cout << "Second dice service instance created (transient behavior)\n";
      dice_service2->roll_dice(99, 3);
    }

    std::cout << "\nDependency Injection example completed successfully!\n";
    std::cout << "\nKey features demonstrated:\n";
    std::cout << "- Service registration with type safety\n";
    std::cout << "- Named service registration and resolution\n";
    std::cout << "- Factory-based service creation\n";
    std::cout << "- Type-safe service resolution with unique_ptr\n";
    std::cout << "- C++23 concepts for compile-time interface validation\n";
    std::cout << "- Direct service registration with type safety\n";

  } catch (const std::runtime_error &e) {
    std::cerr << "DI Error: " << e.what() << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}