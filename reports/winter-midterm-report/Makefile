prefix= /usr/local/apps/tex_live/current/bin/x86_64-linux
LATEX	= $(prefix)/latex -shell-escape
BIBTEX	= $(prefix)/bibtex
DVIPS	= $(prefix)/dvips
DVIPDF  = $(prefix)/dvipdft
XDVI	= $(prefix)/xdvi -gamma 4
GH		= gv

EXAMPLES = $(wildcard *.c)
SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)

pdf: $(PDF)

ps: $(PSF)

$(TRG): %.dvi: %.tex $(EXAMPLES)
	#one way of including source code is to use pygments
	#pygmentize -f latex -o __${EXAMPLES}.tex ${EXAMPLES}
	#requires that you \include{pygments.tex} in your preamble

	$(LATEX) $<
	$(BIBTEX) $(<:%.tex=%)
	$(LATEX) $<
	$(LATEX) $<
	#remove the pygmentized output to avoid cluttering up the directory
	#rm __${SRC}.tex


$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
	ps2pdf $<

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

all: pdf

archive: slides.tar.bz2 progress-report.tar.bz2

slides.tar.bz2: slides.pdf slides.pptx
	rm -f $@
	tar -acf $@ $^

progress-report.tar.bz2: progress-report.pdf progress-report.tex main.bib Makefile
	rm -f $@
	tar -acf $@ $^

clean:
	rm -f *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg

.PHONY: all show clean ps pdf showps

