#include "OrderService.h"

#include "../Domain/Factory/OrderFactory.h"

OrderService::OrderService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo) {
}

Order OrderService::reserve(const std::string& sampleId, const std::string& customerName, int quantity) {
    Order order = OrderFactory::create(sampleId, customerName, quantity, sampleRepo_);
    orderRepo_.save(order);
    return order;
}
