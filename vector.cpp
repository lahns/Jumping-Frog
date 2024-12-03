#include <cstdlib>
#include <cstdio>

template <typename T>
struct Vector {
    T* data;
    unsigned int size;
    unsigned int capacity;
};

// Vector init
template <typename T>
Vector<T>* vector_init() {
    Vector<T>* vector = (Vector<T>*)malloc(sizeof(Vector<T>));
    if (vector == nullptr) {
        printf("Failed to allocate memory for the vector\n");
        return nullptr;
    }
    vector->data = (T*)malloc(sizeof(T));
    if (vector->data == nullptr) {
        free(vector);
        printf("Failed to allocate memory for vector data\n");
        return nullptr;
    }
    vector->size = 0;
    vector->capacity = 1;
    return vector;
}

// Add an element to the back of the vector
template <typename T>
int vector_push_back(Vector<T>* vector, T item) {
    if (vector->size == vector->capacity) {
        vector->capacity *= 2;
        T* new_data = (T*)realloc(vector->data, vector->capacity * sizeof(T));
        if (new_data == nullptr) {
            printf("Failed to allocate memory while resizing\n");
            return -1; // Error
        }
        vector->data = new_data;
    }
    vector->data[vector->size] = item;
    vector->size++;
    return 0; // Success
}

// Access an element at a specific index
template <typename T>
T vector_get(Vector<T>* vector, unsigned int index) {
    if (index >= vector->size) {
        printf("Index out of bounds\n");
        throw; // Error
    }
    return vector->data[index];
}

// Get a pointer to a element inside a vector
template <typename T>
T* vector_get_pointer(Vector<T>* vector, unsigned int index) {
    if (index >= vector->size) {
        printf("Index out of bounds\n");
        throw; // Error
    }
    return &vector->data[index];
}

template <typename T>
void vector_free(Vector<T>* vector) {
    if (vector != nullptr) {
        free(vector->data);
        free(vector);
    }
}

template <typename T>
unsigned int vector_size(Vector<T>* vector) {
    return vector->size;
}

template <typename T>
unsigned int vector_capacity(Vector<T>* vector) {
    return vector->capacity;
}
