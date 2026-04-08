cd libnvme

meson setup .build --prefix=$PWD/install -Dpython=disabled
meson compile -C .build
meson install -C .build
