#!/bin/bash
# fix-headers.sh - convert pragma once to traditional guards

cd "D:\ESP32 projects\hippie safari"

for file in include/*.h src/*.h; do
  # Skip if not a file
  [ -f "$file" ] || continue

  # Get guard name from filename (uppercase, replace / with _)
  guard=$(basename "$file" .h | tr '[:lower:]' '[:upper:]' | tr '/' '_')
  guard="${guard}_H"

  # Create temp file
  tmp="${file}.tmp"

  # Process file
  {
    # Read first line
    read -r first_line < "$file"

    if [[ "$first_line" == "#pragma once" ]]; then
      # Replace pragma once with guards
      echo "#ifndef $guard"
      echo "#define $guard"
      echo ""
      # Skip pragma once line, output rest
      tail -n +2 "$file" | head -n -1
      echo ""
      echo "#endif // $guard"
    else
      # File doesn't have pragma once, skip
      cat "$file"
    fi
  } > "$tmp"

  # Replace original
  mv "$tmp" "$file"
  echo "Fixed: $file (guard: $guard)"
done

echo "Done. All headers now use traditional guards."
