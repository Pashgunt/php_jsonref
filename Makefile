install:
	rm -rf ./build && \
	mkdir build && \
	cd build && \
	cmake .. && \
	make && \
	sudo make install && \
	php --ini && \
	sudo vim /etc/php/*/cli/php.ini 2>/dev/null || echo "php.ini not found"