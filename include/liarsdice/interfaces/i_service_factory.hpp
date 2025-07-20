#pragma once

#include "concepts.hpp"
#include <functional>
#include <memory>
#include <typeindex>

namespace liarsdice::interfaces {

/**
 * @brief Interface for type-erased service factories
 *
 * Provides a common interface for creating services of any type
 * while maintaining type safety through the template system.
 */
class IServiceFactory {
public:
  virtual ~IServiceFactory() = default;

  /**
   * @brief Create a new service instance
   *
   * @return void* Raw pointer to the created service (ownership transferred to caller)
   */
  virtual void *create() = 0;

  /**
   * @brief Get the type index of the service this factory creates
   *
   * @return std::type_index Type index of the service interface
   */
  virtual std::type_index get_type() const = 0;
};

/**
 * @brief Concrete service factory implementation
 *
 * @tparam T Service interface type (must satisfy ServiceInterface concept)
 */
template <ServiceInterface T> class ServiceFactory : public IServiceFactory {
public:
  using FactoryFunction = std::function<std::unique_ptr<T>()>;

  /**
   * @brief Construct factory with custom creation function
   *
   * @param factory Function that creates unique_ptr<T> instances
   */
  explicit ServiceFactory(FactoryFunction factory) : factory_(std::move(factory)) {}

  /**
   * @brief Create a new service instance
   *
   * @return void* Raw pointer to the created service (ownership transferred to caller)
   */
  void *create() override {
    auto ptr = factory_();
    return ptr.release(); // Transfer ownership to void*
  }

  /**
   * @brief Get the type index of the service this factory creates
   *
   * @return std::type_index Type index of the service interface
   */
  std::type_index get_type() const override { return std::type_index(typeid(T)); }

private:
  FactoryFunction factory_;
};

} // namespace liarsdice::interfaces