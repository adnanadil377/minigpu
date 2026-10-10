.PHONY: all build test clean ptx run-examples

all: build

build:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .

test: build
	@cd build && ctest --output-on-failure

run-examples: build
	@echo "=== Vector Addition ==="
	@./build/minigpu --run-cpu examples/vector_add.mgpu
	@echo "=== Elementwise Scaling ==="
	@./build/minigpu examples/scale.mgpu
	@echo "=== Matrix Multiplication ==="
	@./build/minigpu examples/matmul.mgpu

clean:
	@rm -rf build vector_add.ptx scale.ptx matmul.ptx
