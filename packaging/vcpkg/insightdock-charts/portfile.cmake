vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO aavazir/insightdock-charts
    REF "v${VERSION}"
    # Replace with the real hash once the tag exists: `vcpkg install` prints the expected value.
    SHA512 0
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        font-latin INSIGHTDOCK_FONT_LATIN
        podofo     INSIGHTDOCK_WITH_PODOFO
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DINSIGHTDOCK_BUILD_EXAMPLES=OFF
        -DINSIGHTDOCK_BUILD_TESTS=OFF
        -DINSIGHTDOCK_BUILD_BENCHMARKS=OFF
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME InsightDock CONFIG_PATH lib/cmake/InsightDock)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
