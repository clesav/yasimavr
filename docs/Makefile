all: sphinx_html_docs

doxygen_docs: FORCE
	-rmdir dox /S /Q
	@doxygen Doxyfile

sphinx_html_docs: doxygen_docs
	@python make_mock_libs.py
	-cd source/cpp_api && del *.* /Q
	@sphinx-build -M html "source" "build"

clean: FORCE
	-rmdir dox build /S /Q
	-cd source/cpp_api && del *.* /Q

FORCE:
