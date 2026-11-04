from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import cmake_layout


class WalkPdfConan(ConanFile):
    name = "walk_pdf"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        # No dependencies yet; the PDF parsing library is chosen in a later step
        pass

    def validate(self):
        check_min_cppstd(self, "23")

    def layout(self):
        cmake_layout(self)
