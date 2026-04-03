#!/bin/bash

set -e

EXERCISES_DIR="./exercises"
OUT_DIR="./output"
TMP_DIR="/tmp/record_build"

mkdir -p "$OUT_DIR" "$TMP_DIR"

# ─────────────────────────────────────────────
# HELPER: extract a yaml field from frontmatter
# ─────────────────────────────────────────────
get_yaml_field() {
    local file=$1
    local field=$2
    awk '/^---$/{p++} p==1{print} /^---$/ && p==2{exit}' "$file" \
        | grep "^${field}:" | sed "s/^${field}:[[:space:]]*//"
}

# ─────────────────────────────────────────────
# HELPER: expand { include <path> lang=<lang> }
# ─────────────────────────────────────────────
expand_includes() {
    local file=$1
    local original_dir=$2

    while IFS= read -r line; do
        if echo "$line" | grep -qE '^\{ include .+ lang=.+ \}$'; then
            inc_path=$(echo "$line" | sed 's/^{ include \(.*\) lang=[^ ]* }$/\1/')
            inc_lang=$(echo "$line" | sed 's/^{ include .* lang=\([^ ]*\) }$/\1/')

            if [[ "$inc_path" = /* ]]; then
                abs_path="$inc_path"
            else
                abs_path="$original_dir/$inc_path"
            fi

            if [ -f "$abs_path" ]; then
                printf '```%s\n' "$inc_lang"
                cat "$abs_path"
                printf '\n```\n\n'
            else
                echo "> **Error:** File not found: \`$abs_path\`"
            fi
        else
            echo "$line"
        fi
    done < "$file"
}

# ─────────────────────────────────────────────
# STEP 0: Build cover page
# ─────────────────────────────────────────────
echo "Building cover page..."

COVER_MD="$TMP_DIR/cover.md"

cat > "$COVER_MD" <<EOF
---
header-includes:
  - |
    \usepackage{geometry}
    \usepackage{graphicx}
    \usepackage{array}
    \usepackage{tabularx}
  - \pagestyle{empty}
---
EOF

cat >> "$COVER_MD" << 'COVEREOF'
\begin{center}

{\large\textbf{COIMBATORE INSTITUTE OF TECHNOLOGY}}

\vspace{0.2cm}

{\small\textit{(Government Aided Autonomous Institution Approved by AICTE, New Delhi \& Affiliated to Anna University, Chennai)}}

\vspace{3cm}

\includegraphics[width=3cm]{./cit.png}

\vspace{0.8cm}

{\large\textbf{Department of Computing}}

\vspace{0.3cm}

{\large\textit{Internet of Things Laboratory}}

\vspace{0.2cm}

{\large\textit{(20MSS84)}}

\end{center}

\vspace{\fill}

\begin{flushright}
\renewcommand{\arraystretch}{1.6}
\begin{tabular}{|p{8cm}|}
\hline
\textbf{NAME:} SHREE KOTTES J \\
\textbf{REG.NO:} 7176 22 31 050 \\
\textbf{CLASS:} M.Sc., SOFTWARE SYSTEMS \\
\hspace{1.8cm} \textit{(8\textsuperscript{th} Semester)} \\
\textbf{MAIL ID:} 71762231050@cit.edu.in \\
\hline
\end{tabular}
\end{flushright}
COVEREOF

pandoc "$COVER_MD" \
    --pdf-engine=xelatex \
    -V geometry:"top=1in,bottom=1in,left=1in,right=1in" \
    -V papersize=a4 \
    -V fontsize=12pt \
    --standalone \
    -o "$OUT_DIR/cover.pdf"

echo "  → Built cover.pdf"

# ─────────────────────────────────────────────
# STEP 1: Process each .md and build PDFs
# ─────────────────────────────────────────────
declare -A EX_MAP
INDEX_ROWS=""

for md_file in "$EXERCISES_DIR"/*.md; do
    filename=$(basename "$md_file" .md)

    ex_no=$(get_yaml_field "$md_file" "ex_no")
    ex_title=$(get_yaml_field "$md_file" "ex_title")
    ex_date=$(get_yaml_field "$md_file" "date")

    if [ -z "$ex_no" ]; then
        echo "Skipping $filename: no ex_no found"
        continue
    fi

    echo "Processing ex$ex_no: $ex_title ..."

    tmp_md="$TMP_DIR/${filename}_expanded.md"

    # ── Strip frontmatter ──
    awk '
        /^---$/ { delim++; next }
        delim < 2 { next }
        { print }
    ' "$md_file" > "$TMP_DIR/${filename}_body.md"

    # ── Expand { include } directives ──
    original_dir=$(dirname "$md_file")
    expand_includes "$TMP_DIR/${filename}_body.md" "$original_dir" > "$TMP_DIR/${filename}_content.md"

    # ── Inject frontmatter + table ──
    cat > "$tmp_md" <<EOF
---
header-includes:
  - |
    \usepackage{needspace}
    \let\oldsection\section
    \renewcommand{\section}{\needspace{8\baselineskip}\oldsection}
    \let\oldsubsection\subsection
    \renewcommand{\subsection}{\needspace{8\baselineskip}\oldsubsection}
    \let\oldsubsubsection\subsubsection
    \renewcommand{\subsubsection}{\needspace{8\baselineskip}\oldsubsubsection}
    \usepackage{multirow}
    \usepackage{array}
    \usepackage{graphicx}
    \usepackage{fvextra}
    \DefineVerbatimEnvironment{Highlighting}{Verbatim}{breaklines,commandchars=\\\\\{\}}
  - \def\exno{$ex_no}
  - \def\extitle{$ex_title}
  - \def\exdate{$ex_date}
  - \pagestyle{empty}
---
EOF

    # ── Write table separately to avoid heredoc backslash issues ──
    cat >> "$tmp_md" << 'TABLEEOF'
\begin{center}
\setlength{\arrayrulewidth}{0.5pt}
\renewcommand{\arraystretch}{2.5}
\begin{tabular}{|p{3cm}|>{\centering\arraybackslash}m{\dimexpr\linewidth-3cm-4\tabcolsep-3\arrayrulewidth\relax}|}
\hline
\centering \textbf{EX.NO: \exno} &
\multirow{2}{=}{\centering\large\textbf{\MakeUppercase{\extitle}}} \\ \cline{1-1}
\centering \textbf{\exdate} & \\ \hline
\end{tabular}
\end{center}

TABLEEOF

    cat "$TMP_DIR/${filename}_content.md" >> "$tmp_md"

    # ── Compile to PDF ──
    out_pdf="$OUT_DIR/${filename}.pdf"
    pandoc "$tmp_md" \
        --pdf-engine=xelatex\
        --highlight-style=tango \
        -V geometry:"top=1in,bottom=1in,left=1in,right=1in" \
        -V papersize=a4 \
        -V fontsize=12pt \
        -V linestretch=1 \
        --standalone \
        -o "$out_pdf"

    EX_MAP["$ex_no"]="$out_pdf"
    INDEX_ROWS+="| $ex_no | $ex_title | $ex_date |\n"

    echo "  → Built $out_pdf"
done

# ─────────────────────────────────────────────
# STEP 2: Build index and compile to PDF
# ─────────────────────────────────────────────
echo "Building index..."

INDEX_MD="$TMP_DIR/index.md"

cat > "$INDEX_MD" <<EOF
---
header-includes:
  - |
    \usepackage{array}
    \usepackage{booktabs}
  - \pagestyle{empty}
---
\begin{center}
{\Large\textbf{INDEX}}
\end{center}

\vspace{0.5cm}

EOF

cat >> "$INDEX_MD" << 'INDEXEOF'
\begin{center}
\setlength{\arrayrulewidth}{0.5pt}
\renewcommand{\arraystretch}{2}
\begin{tabular}{|>{\centering\arraybackslash}p{1cm}|>{\centering\arraybackslash}p{2.5cm}|>{\centering\arraybackslash}m{\dimexpr\linewidth-1cm-2.5cm-2.5cm-8\tabcolsep-5\arrayrulewidth\relax}|>{\centering\arraybackslash}p{2.5cm}|}
\hline
\textbf{S. No} & \textbf{Date} & \textbf{Title} & \textbf{Sign} \\
\hline
INDEXEOF

for ex_no in $(echo "${!EX_MAP[@]}" | tr ' ' '\n' | sort -n); do
    title=$(get_yaml_field "$EXERCISES_DIR/$(basename "${EX_MAP[$ex_no]}" .pdf).md" "ex_title")
    date=$(get_yaml_field "$EXERCISES_DIR/$(basename "${EX_MAP[$ex_no]}" .pdf).md" "date")
    echo "$ex_no. & $date & $title & \\\\" >> "$INDEX_MD"
    echo "\\hline" >> "$INDEX_MD"
done

cat >> "$INDEX_MD" << 'INDEXEOF'
\end{tabular}
\end{center}
INDEXEOF

pandoc "$INDEX_MD" \
    --pdf-engine=xelatex \
    -V geometry:"top=1in,bottom=1in,left=1in,right=1in" \
    -V papersize=a4 \
    -V fontsize=12pt \
    -V linestretch=1 \
    --standalone \
    -o "$OUT_DIR/index.pdf"

echo "  → Built index.pdf"

# ─────────────────────────────────────────────
# STEP 3: Merge all PDFs ordered by ex_no
# ─────────────────────────────────────────────
echo "Merging into record.pdf..."

ORDERED_PDFS="$OUT_DIR/cover.pdf $OUT_DIR/index.pdf"
for ex_no in $(echo "${!EX_MAP[@]}" | tr ' ' '\n' | sort -n); do
    ORDERED_PDFS="$ORDERED_PDFS ${EX_MAP[$ex_no]}"
done

if command -v pdfunite &> /dev/null; then
    pdfunite $ORDERED_PDFS "$OUT_DIR/record.pdf"
elif command -v pdftk &> /dev/null; then
    pdftk $ORDERED_PDFS cat output "$OUT_DIR/record.pdf"
else
    echo "Error: install pdfunite (poppler-utils) or pdftk to merge PDFs"
    exit 1
fi

echo "  → Built record.pdf"

# ─────────────────────────────────────────────
# Cleanup
# ─────────────────────────────────────────────
rm -rf "$TMP_DIR"
echo ""
echo "Done! Output: $OUT_DIR/record.pdf"

