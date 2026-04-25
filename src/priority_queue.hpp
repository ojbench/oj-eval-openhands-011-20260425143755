#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
    struct Node {
        T data;
        Node *left;
        Node *right;
        int npl;  // null path length
        
        Node(const T &value) : data(value), left(nullptr), right(nullptr), npl(0) {}
    };
    
    Node *root;
    Compare cmp;
    size_t currentSize;
    
    // Helper functions
    Node* copyNode(Node *other) {
        if (other == nullptr) return nullptr;
        Node *newNode = new Node(other->data);
        newNode->left = copyNode(other->left);
        newNode->right = copyNode(other->right);
        newNode->npl = other->npl;
        return newNode;
    }
    
    void deleteNode(Node *node) {
        if (node == nullptr) return;
        deleteNode(node->left);
        deleteNode(node->right);
        delete node;
    }
    
    int getNpl(Node *node) const {
        return (node == nullptr) ? -1 : node->npl;
    }
    
    Node* mergeNodes(Node *h1, Node *h2) {
        if (h1 == nullptr) return h2;
        if (h2 == nullptr) return h1;
        
        // Ensure h1 has the larger root (max-heap)
        if (cmp(h1->data, h2->data)) {
            Node *temp = h1;
            h1 = h2;
            h2 = temp;
        }
        
        // Merge h1's right subtree with h2
        h1->right = mergeNodes(h1->right, h2);
        
        // Maintain leftist heap property
        if (getNpl(h1->left) < getNpl(h1->right)) {
            Node *temp = h1->left;
            h1->left = h1->right;
            h1->right = temp;
        }
        
        // Update npl
        h1->npl = (h1->right == nullptr) ? 0 : h1->right->npl + 1;
        
        return h1;
    }
    
    // Exception-safe merge helper
    Node* mergeNodesSafe(Node *h1, Node *h2) {
        if (h1 == nullptr) return h2;
        if (h2 == nullptr) return h1;
        
        // Create copies for exception safety
        Node *h1Copy = copyNode(h1);
        Node *h2Copy = copyNode(h2);
        
        try {
            Node *result = mergeNodes(h1Copy, h2Copy);
            return result;
        } catch (...) {
            deleteNode(h1Copy);
            deleteNode(h2Copy);
            throw;
        }
    }

public:
    /**
     * @brief default constructor
     */
    priority_queue() : root(nullptr), currentSize(0) {}

    /**
     * @brief copy constructor
     * @param other the priority_queue to be copied
     */
    priority_queue(const priority_queue &other) : root(nullptr), currentSize(0), cmp(other.cmp) {
        root = copyNode(other.root);
        currentSize = other.currentSize;
    }

    /**
     * @brief deconstructor
     */
    ~priority_queue() {
        deleteNode(root);
    }

    /**
     * @brief Assignment operator
     * @param other the priority_queue to be assigned from
     * @return a reference to this priority_queue after assignment
     */
    priority_queue &operator=(const priority_queue &other) {
        if (this == &other) return *this;
        
        // Create copy first for exception safety
        Node *newRoot = copyNode(other.root);
        size_t newSize = other.currentSize;
        Compare newCmp = other.cmp;
        
        // If successful, delete old data and assign new
        deleteNode(root);
        root = newRoot;
        currentSize = newSize;
        cmp = newCmp;
        
        return *this;
    }

    /**
     * @brief get the top element of the priority queue.
     * @return a reference of the top element.
     * @throws container_is_empty if empty() returns true
     */
    const T & top() const {
        if (empty()) {
            throw container_is_empty();
        }
        return root->data;
    }

    /**
     * @brief push new element to the priority queue.
     * @param e the element to be pushed
     */
    void push(const T &e) {
        Node *newNode = new Node(e);
        
        try {
            root = mergeNodes(root, newNode);
            currentSize++;
        } catch (...) {
            delete newNode;
            throw runtime_error();
        }
    }

    /**
     * @brief delete the top element from the priority queue.
     * @throws container_is_empty if empty() returns true
     */
    void pop() {
        if (empty()) {
            throw container_is_empty();
        }
        
        // Save the root data for potential restoration
        T rootData = root->data;
        Node *left = root->left;
        Node *right = root->right;
        int rootNpl = root->npl;
        delete root;
        
        try {
            root = mergeNodes(left, right);
            currentSize--;
        } catch (...) {
            // Restore the deleted node
            root = new Node(rootData);
            root->left = left;
            root->right = right;
            root->npl = rootNpl;
            throw runtime_error();
        }
    }

    /**
     * @brief return the number of elements in the priority queue.
     * @return the number of elements.
     */
    size_t size() const {
        return currentSize;
    }

    /**
     * @brief check if the container is empty.
     * @return true if it is empty, false otherwise.
     */
    bool empty() const {
        return root == nullptr;
    }

    /**
     * @brief merge another priority_queue into this one.
     * The other priority_queue will be cleared after merging.
     * The complexity is at most O(logn).
     * @param other the priority_queue to be merged.
     */
    void merge(priority_queue &other) {
        if (this == &other) return;
        
        // Save pointers for potential restoration
        Node *oldRoot1 = root;
        Node *oldRoot2 = other.root;
        size_t oldSize1 = currentSize;
        size_t oldSize2 = other.currentSize;
        
        try {
            root = mergeNodes(root, other.root);
            currentSize += other.currentSize;
            
            // Clear other queue
            other.root = nullptr;
            other.currentSize = 0;
        } catch (...) {
            // Restore both original states
            root = oldRoot1;
            currentSize = oldSize1;
            other.root = oldRoot2;
            other.currentSize = oldSize2;
            throw runtime_error();
        }
    }
};

}

#endif