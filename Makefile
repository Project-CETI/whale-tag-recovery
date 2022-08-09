native:
	/bin/zsh -c "cmake -B build/ && cd build/ && make -j4"

#
# Pico
#

pico-image:
	docker build -t ceti\:pico -f $(shell pwd)/docker/pico/Picofile $(shell pwd)/docker/pico

pico-dev:
	docker run --rm -v $(shell pwd)\:/project ceti\:pico
	sudo chown -R $(shell id -u)\:$(shell id -g) build

pico-dbg:
	docker run --rm -itp 80\:80 -v $(shell pwd)\:/project --name dev-debug-pico ceti\:pico bash
pico:
	$(MAKE) pico-dev || $(MAKE) pico-image pico-dev
clean-pico:
	docker rmi ceti\:pico

clean-dev:
	rm -r build/

#
# Docs
#

docs-image:
	docker build -t ceti\:docs -f $(shell pwd)/docker/docs/Docsfile .

docs-server:
	docker run --rm -dp 8080:80 --name ceti-docs ceti\:docs

docs: docs-image docs-server

stop-docs:
	-docker kill ceti-docs

clean-docs: stop-docs
	docker rmi ceti\:docs

#
# Clean globs
#

clean-docker: clean-pico clean-docs

clean: clean-dev clean-docker

#
# All target glob
#

all:
	pico docs
