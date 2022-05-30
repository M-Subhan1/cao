idf_component_register(
    SRCS "main.c" "app.c" "timer.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES certificate.pem
)