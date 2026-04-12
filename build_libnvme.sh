cd libnvme

meson setup .build --prefix=$PWD/install -Dpython=disabled --default-library=static
meson compile -C .build
meson install -C .build
