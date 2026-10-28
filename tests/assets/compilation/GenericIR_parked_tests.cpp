#include "../../tester.hpp"
#include "novac/assets/compilation/ir/GenericIR.hpp"
#include <string>
using namespace novac;

TEST(Extensibility, GenericIrAcceptsRegisteredCustomOperationsAndMultipleResults) {
    ir::generic::OperationRegistry registry;
    registry.add({"example.split", 1, 1, 2, 2, {"pure"}, {}});

    ir::generic::Operation operation;
    operation.name = "example.split";
    operation.operands = {{0}};
    operation.results = {{1}, {2}};
    operation.resultTypes = {{"example.left"}, {"example.right"}};
    operation.metadata.set<std::string>("example.note", "custom");

    registry.validate(operation);
    CHECK(operation.metadata.require<std::string>("example.note") == "custom");
}


