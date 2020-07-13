from distutils.core import setup, Extension

def main():
    setup(name="luci",
          version="1.0.0",
          description="Python interface for the LuCi C library parser",
          author="<your name>",
          author_email="your_email@gmail.com",
          ext_modules=[Extension("luci", ["lucimodule.c"])])

if __name__ == "__main__":
    main()

