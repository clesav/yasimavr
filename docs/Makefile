all: sphinx_html_docs

doxygen_docs:
	-cd dox && rmdir html xml /S /Q
	@doxygen Doxyfile

sphinx_html_docs: doxygen_docs
	python make_mock_libs.py
	-cd source/cpp_api && del *.* /Q
	@sphinx-build -M html "source" "build"

FORCE:
