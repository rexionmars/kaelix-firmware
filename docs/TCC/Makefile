# TCC --- compilação completa.
#
# Ordem: pdflatex -> bibtex -> makeglossaries (siglas) -> makeindex (símbolos)
#        -> pdflatex -> pdflatex.
# As figuras NÃO são geradas aqui: vêm de ../../experiments/figures/output/,
# regeneradas pelo pipeline em experiments/figures/scripts/.

DOC := main
TEX := pdflatex -interaction=nonstopmode -halt-on-error -output-directory=build
BUILD_DIR := build

.PHONY: all clean

all: $(BUILD_DIR)/$(DOC).pdf

# Depende também dos arquivos em subpastas (capítulos, preâmbulo, pré-textuais):
# com só *.tex, editar um capítulo não disparava nova compilação.
# O mkdir fica na receita, e não numa regra própria, porque build/ já existe
# no repositório (.gitkeep): o \include de capitulos/ grava capitulos/*.aux
# dentro de build/, e o pdflatex não cria subdiretórios.
#
# O rm remove restos de compilação fora de build/: o bibtex procura capitulos/*.aux
# primeiro aqui, e uma cópia velha esconde as citações novas dos capítulos.
$(BUILD_DIR)/$(DOC).pdf: $(wildcard *.tex */*.tex *.bst) referencias.bib $(wildcard imagens/*)
	mkdir -p $(BUILD_DIR)/capitulos
	rm -f capitulos/*.aux
	$(TEX) $(DOC)
	bibtex $(BUILD_DIR)/$(DOC)
	makeglossaries -d $(BUILD_DIR) $(DOC)
	makeindex $(BUILD_DIR)/$(DOC).nlo -s nomencl.ist -o $(BUILD_DIR)/$(DOC).nls
	$(TEX) $(DOC)
	$(TEX) $(DOC)

clean:
	rm -rf $(BUILD_DIR)