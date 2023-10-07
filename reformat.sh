
fix() {
  echo fixing $1
  clang-format $1 >tmp
  mv tmp $1
}

for dir in *.c *.h; do fix $dir; done
