from conans import ConanFile
from conan.tools.gnu import Autotools

class Adftool(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    name = "adftool"
    # FIXME: Don’t hardcode the python version (also, change the
    # PYTHONPATH after installation, see later)
    requires = "gettext/[>0.0.0]", "hdf5/[>0.0.0]", "gmp/[>0.0.0]", "cpython/3.10.0", "zlib/[>0.0.0]"
    version = "@VERSION@"
    generators = "AutotoolsToolchain", "AutotoolsDeps"
    exports_sources = "*"

    def build(self):
        autotools = Autotools(self)
        autotools.configure(args=["--enable-relocatable"])
        autotools.make()
        autotools.install()

    def package_info(self):
        # FIXME: Don’t hardcode 3.10 here.
        self.env_info.PYTHONPATH.append(self.package_folder + '/lib/python3.10/site-packages')
