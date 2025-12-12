#!/bin/bash

# Script para probar el método DELETE del servidor webserv

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Test DELETE Method ===${NC}\n"

# Crear un archivo de prueba
TEST_FILE="test_delete_file.txt"
echo "Este es un archivo de prueba para DELETE" > $TEST_FILE
echo -e "${GREEN}✓${NC} Archivo de prueba creado: $TEST_FILE\n"

# Mostrar que el archivo existe
if [ -f "$TEST_FILE" ]; then
    echo -e "${GREEN}✓${NC} Archivo existe antes de DELETE"
    ls -lh $TEST_FILE
    echo ""
fi

# Hacer la petición DELETE
echo -e "${YELLOW}Enviando petición DELETE...${NC}"
RESPONSE=$(curl -X DELETE http://localhost:8080/$TEST_FILE -w "\nHTTP_CODE:%{http_code}" -s)
HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE" | cut -d: -f2)

echo -e "Código de respuesta: ${YELLOW}$HTTP_CODE${NC}"

# Verificar el código de respuesta
if [ "$HTTP_CODE" = "204" ] || [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}✓${NC} Respuesta exitosa"
else
    echo -e "${RED}✗${NC} Respuesta inesperada"
fi

# Verificar que el archivo fue eliminado
echo ""
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${GREEN}✓${NC} Archivo eliminado correctamente"
else
    echo -e "${RED}✗${NC} El archivo todavía existe"
    ls -lh $TEST_FILE
fi

echo -e "\n${YELLOW}=== Test de archivo inexistente ===${NC}\n"

# Probar DELETE en un archivo que no existe
echo "Intentando DELETE en archivo inexistente..."
RESPONSE=$(curl -X DELETE http://localhost:8080/archivo_inexistente.txt -w "\nHTTP_CODE:%{http_code}" -s)
HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE" | cut -d: -f2)

echo -e "Código de respuesta: ${YELLOW}$HTTP_CODE${NC}"

if [ "$HTTP_CODE" = "404" ]; then
    echo -e "${GREEN}✓${NC} 404 Not Found - Correcto"
else
    echo -e "${RED}✗${NC} Se esperaba 404, se obtuvo $HTTP_CODE"
fi

echo -e "\n${YELLOW}=== Tests completados ===${NC}"
