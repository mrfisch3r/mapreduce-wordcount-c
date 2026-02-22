#!/usr/bin/env bash
set -euo pipefail

make

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

mkdir -p "$tmp/nested"
cat > "$tmp/a.txt" <<'EOF'
Apple apple banana.
EOF
cat > "$tmp/nested/b.txt" <<'EOF'
banana BANANA grape
EOF

out="$(./wordcount "$tmp")"

echo "$out" | grep -E '^apple 2$'
echo "$out" | grep -E '^banana 3$'
echo "$out" | grep -E '^grape 1$'

echo "PASS test_small"
