idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES gamebuino aka_runtime platform_aka
    PRIV_REQUIRES app_update)
