echo #!/bin/sh

if [ -z "$1" ]; then
    echo "Ошибка: не указан файл" >&2
    exit 1
fi

SOURCE_FILE="$1"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Ошибка: файл '$SOURCE_FILE' не найден" >&2
    exit 2
fi

SOURCE_DIR=$(cd "$(dirname "$SOURCE_FILE")" && pwd)
REAL_SOURCE_PATH="$SOURCE_DIR/$(basename "$SOURCE_FILE")"

OUTPUT_NAME=$(grep -i "Output:" "$REAL_SOURCE_PATH" | sed -E 's/.*[Oo]utput:[[:space:]]*([^[:space:]]+).*/\1/' | head -n 1)

if [ -z "$OUTPUT_NAME" ]; then
    echo "Ошибка: в файле не найден комментарий 'Output:'" >&2
    exit 3
fi

TMP_DIR=$(mktemp -d)
if [ ! -d "$TMP_DIR" ]; then
    echo "Ошибка: не удалось создать временный каталог" >&2
    exit 4
fi

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT HUP INT QUIT TERM

cp "$REAL_SOURCE_PATH" "$TMP_DIR/"
cd "$TMP_DIR" || exit 5

FILE_EXT="${SOURCE_FILE##*.}"
SUCCESS=0

case "$FILE_EXT" in
    c|cpp)
        COMPILER="gcc"
        [ "$FILE_EXT" = "cpp" ] && COMPILER="g++"
        
        if ! $COMPILER "$(basename "$SOURCE_FILE")" -o "$OUTPUT_NAME" 2>build_err.log; then
            echo "Ошибка компиляции C/C++" >&2
            cat build_err.log >&2
            SUCCESS=0
        else
            SUCCESS=1
        fi
        ;;
    *)
        echo "Ошибка: неподдерживаемое расширение файла" >&2
        exit 6
        ;;
esac

if [ "$SUCCESS" -eq 1 ] && [ -f "$OUTPUT_NAME" ]; then
    mv "$OUTPUT_NAME" "$SOURCE_DIR/"
    echo "Сборка успешно завершена. Файл сохранен как: $SOURCE_DIR/$OUTPUT_NAME"
    exit 0
else
    echo "Сборка завершилась неудачно" >&2
    exit 7
fi
EOF
