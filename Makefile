all:
	cmake ./ -GNinja && ninja && sudo ninja install