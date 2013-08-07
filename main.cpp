#include "spinner/vector.hpp"
#include "boomstick/geom2D.hpp"

int main(int argc, char **argv) {
	spn::AVec4 v(1,2,3,4);
    return v.dot(v);
}
