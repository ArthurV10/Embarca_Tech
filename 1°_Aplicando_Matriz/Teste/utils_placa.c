#include <stdio.h>

void invert_matrix_vertical(uint8_t* matrix, int rows, int cols) {
    for (int i = 0; i < rows / 2; i++) {
        for (int j = 0; j < cols; j++) {
            uint8_t temp = matrix[i * cols + j];
            matrix[i * cols + j] = matrix[(rows - i - 1) * cols + j];
            matrix[(rows - i - 1) * cols + j] = temp;
        }
    }
}

// Função para inverter apenas as linhas pares da matriz horizontalmente 
void invert_matrix_horizontal_even_rows(uint8_t* matrix, int rows, int cols) { 
    for (int i = 0; i < rows; i += 2) {
         // Incrementa de 2 em 2 para pegar apenas as linhas pares (0, 2, 4, ...)
        for (int j = 0; j < cols / 2; j++) {
            uint8_t temp = matrix[i * cols + j];
            matrix[i * cols + j] = matrix[i * cols + (cols - j - 1)];
            matrix[i * cols + (cols - j - 1)] = temp; 
        }
    } 
}

// Função para os desenhos funcionar normalmente, e placa ficar corretamente ajustada conforme as imagens da documentação
void fixingBitDogLab(uint8_t* matrix, int rows, int cols){
    invert_matrix_vertical(matrix, rows, cols);
    invert_matrix_horizontal_even_rows(matrix, rows, cols);
}