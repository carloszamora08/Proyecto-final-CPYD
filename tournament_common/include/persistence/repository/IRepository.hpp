//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_IREPOSITORY_HPP
#define RESTAPI_IREPOSITORY_HPP
#include <vector>
#include <memory>
#include <expected>

template<typename Type, typename Id, typename expectedId>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual expectedId Create (const Type & entity) = 0;
    virtual std::expected<std::vector<std::shared_ptr<Type>>, std::string> ReadAll() = 0;
    virtual std::expected<std::shared_ptr<Type>, std::string> ReadById(Id id) = 0;
    virtual expectedId Update (Id id, const Type & entity) = 0;
    virtual std::expected<void, std::string> Delete(Id id) = 0;
};
#endif //RESTAPI_IREPOSITORY_HPP