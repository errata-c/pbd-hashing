#include "Context.hpp"
#include <memory>

int main() {
	std::unique_ptr<Context> ctx(new Context(1000, 600));
	return ctx->run();
}