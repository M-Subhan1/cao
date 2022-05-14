idf_component_register(
    SRCS "main.c" "config.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES certificate.pem
)