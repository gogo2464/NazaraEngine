package("qt5gui")

    set_homepage("https://www.qt.io")
    set_description("Qt is the faster, smarter way to create innovative devices, modern UIs & applications for multiple screens. Cross-platform software development at its best.")
    set_license("LGPL-3")

    on_load(function (package)
        package:add("deps", "qt5base", "qt5core", {debug = package:is_debug()})
    end)

    on_fetch(function (package)
        local base = package:dep("qt5base")
        local qt = base:fetch()
        if not qt then
            return
        end

        return {
            includedirs = {qt.includedir, path.join(qt.includedir, "QtGui")},
            links = table.wrap("Qt5Gui" .. (package:is_plat("windows") and package:is_debug() and "d" or "")),
            linkdirs = table.wrap(qt.libdir),
        }
    end)

    on_install(function (package)
        local base = package:dep("qt5base")
        local qt = base:fetch()
        assert(qt, "qt5base is required")
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            int test(int argc, char** argv) {
                QGuiApplication app (argc, argv);
                return app.exec();
            }
        ]]}, {configs = {languages = "c++14", cxflags = not package:is_plat("windows") and "-fPIC" or nil}, includes = {"QGuiApplication"}}))
    end)
