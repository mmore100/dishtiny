# Set up commands
.PHONY : default native web all ndata debug static sanitize profile debug-web web-debug clean cleanlogs cleanall print-% serve

# Project-specific settings
PROJECT := dishtiny
EMP_DIR := third-party/Empirical/source

DISHTINY_HASH := $(shell git rev-parse --short HEAD)
DISHTINY_DIRTY := $(shell \
    ( git diff-index --quiet HEAD -- && echo "-clean" || echo "-dirty" ) \
    | tr -d '\040\011\012\015' \
  )
# to compile different metrics/selecctors
# make ARGS="-DMETRIC=streak -DSELECTOR=roulette"

# Flags to use regardless of compiler
CFLAGS_all := -std=c++17 -Wall -Wno-unused-function -Wno-unused-private-field \
  -Iinclude -Ithird-party/ -DDISHTINY_HASH_=$(DISHTINY_HASH)$(DISHTINY_DIRTY) \
	$(ARGS)

# Native compiler information
CXX := h5c++
CFLAGS_nat := -O3 -DNDEBUG $(CFLAGS_all) -fopenmp
CFLAGS_nat_ndata = $(CFLAGS_nat) -DNDATA
CFLAGS_nat_debug := -g -DEMP_TRACK_MEM -fopenmp $(CFLAGS_all)
CFLAGS_nat_sanitize := -fsanitize=address -fsanitize=undefined $(CFLAGS_nat_debug)
CFLAGS_nat_profile := -pg -DNDEBUG -fopenmp $(CFLAGS_all)

# Emscripten compiler information
CXX_web := emcc
OFLAGS_web_all := -s ALLOW_MEMORY_GROWTH=1 -s USE_ZLIB=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" -s TOTAL_MEMORY=10485760 --js-library $(EMP_DIR)/web/library_emp.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=1 -s ABORTING_MALLOC=0 -s
OFLAGS_web := -O3 -DNDEBUG
OFLAGS_web_debug := -g4 -Wno-dollar-in-identifier-extension -s DEMANGLE_SUPPORT=1 -s ASSERTIONS=1

CFLAGS_web := $(CFLAGS_all) $(OFLAGS_web) $(OFLAGS_web_all)
CFLAGS_web_debug := $(CFLAGS_all) $(OFLAGS_web_debug) $(OFLAGS_web_all)

default: $(PROJECT)
native: $(PROJECT)
web: $(PROJECT).js $(PROJECT).html
all: $(PROJECT) $(PROJECT).js $(PROJECT).html

debug:	CFLAGS_nat := $(CFLAGS_nat_debug)
debug:	$(PROJECT)

debug-web:	CFLAGS_web := $(CFLAGS_web_debug)
debug-web:	$(PROJECT).js $(PROJECT).html

web-debug:	debug-web

$(PROJECT):	source/native.cpp include/
	@echo CXX $(CXX)
	$(CXX) $(CFLAGS_nat) source/native.cpp -o $(PROJECT)
	@echo To build the web version use: make web

$(PROJECT).js: source/web.cpp include/
	cd third-party/emsdk && . ./emsdk_env.sh && cd - && $(CXX_web) $(CFLAGS_web) source/web.cpp -o web/$(PROJECT).js

$(PROJECT).html: web/includes
	python3 web/make_html.py

docs:
	cd docs && make html

serve:
	python3 -m http.server

docs/_build/doc-coverage.json:
	cd docs && make coverage

documentation-coverage-badge.json: docs/_build/doc-coverage.json
	python3 ci/parse_documentation_coverage.py docs/_build/doc-coverage.json > web/documentation-coverage-badge.json

version-badge.json:
	python3 ci/parse_version.py .bumpversion.cfg > web/version-badge.json

doto-badge.json:
	python3 ci/parse_dotos.py $$(./ci/grep_dotos.sh) > web/doto-badge.json

badges: documentation-coverage-badge.json version-badge.json doto-badge.json

clean:
	rm -f $(PROJECT) web/$(PROJECT).js web/*.js.map web/*.js.map *~ source/*.o web/*.wasm web/*.wast

test: debug web
	timeout 30 ./dishtiny | grep -q "^32$$" && echo 'matched!' || exit 1
	npm install
	echo "const puppeteer = require('puppeteer'); var express = require('express'); var app = express(); app.use(express.static('web')); app.listen(3000); express.static.mime.types['wasm'] = 'application/wasm'; function sleep(millis) { return new Promise(resolve => setTimeout(resolve, millis)); } async function run() { const browser = await puppeteer.launch(); const page = await browser.newPage(); await page.goto('http://localhost:3000/dishtiny.html'); await sleep(30000); const html = await page.content(); console.log(html); browser.close(); process.exit(0); } run();" | node | grep -q "Update 0" && echo "matched!" ||  exit 1
	echo "const puppeteer = require('puppeteer'); var express = require('express'); var app = express(); app.use(express.static('web')); app.listen(3000); express.static.mime.types['wasm'] = 'application/wasm'; function sleep(millis) { return new Promise(resolve => setTimeout(resolve, millis)); } async function run() { const browser = await puppeteer.launch(); const page = await browser.newPage(); page.on('console', msg => console.log(msg.text())); await page.goto('http://localhost:3000/dishtiny.html'); await sleep(30000); await page.content(); browser.close(); process.exit(0); } run();" | node | grep -q "web viewer load SUCCESS" && echo "matched!"|| exit 1

tests:
	cd tests && make
	cd tests && make opt
	cd tests && make fulldebug

coverage:
	cd tests && make coverage

install-test-dependencies:
	git submodule update --init && cd third-party && bash ./install_emsdk.sh && bash ./install_force_cover.sh

.PHONY: tests clean test serve debug native web tests install-test-dependencies documentation-coverage documentation-coverage-badge.json version-badge.json doto-badge.json
