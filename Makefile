pico-image:
	docker build -t ceti\:pico -f ./docker/pico/Picofile ./docker/pico

pico-dev:
	docker run --rm -v $(shell pwd)\:/project ceti\:pico
	sudo chown -R $(shell id -u)\:$(shell id -g) build

pico-dbg:
	docker run --rm -itp 80\:80 -v $(shell pwd)\:/project --name dev-debug-pico ceti\:pico bash
