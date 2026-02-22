#!/usr/bin/env bash
set -euo pipefail

make

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

mkdir -p "$tmp/d1" "$tmp/d2"
cat > "$tmp/d1/a.txt" <<'EOF'
alpha beta beta
EOF
cat > "$tmp/d2/b.txt" <<'EOF'
beta gamma
EOF

out="$(./wordcount "$tmp/d1" "$tmp/d2")"

echo "$out" | grep -E '^alpha 1$'
echo "$out" | grep -E '^beta 3$'
echo "$out" | grep -E '^gamma 1$'

echo "PASS test_multiargs"
