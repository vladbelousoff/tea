#pragma once

#include <stdbool.h>

/**
 * @brief Get the struct for this entry.
 * @param address The address of the list entry structure.
 * @param type The type of the struct this is embedded in.
 * @param field The name of the list_entry within the struct.
 * @return A pointer to the containing structure.
 */
#define tea_list_record(address, type, field)                                  \
  ((type *)((char *)(address) - (char *)(&((type *)0)->field)))

/**
 * @brief Iterate over a list.
 * @param position The &struct tea_list_entry to use as a loop cursor.
 * @param head The head for your list.
 */
#define tea_list_for_each(position, head)                                      \
  for (position = (head)->next; position != head; position = position->next)

/**
 * @brief Iterate over a list safe against removal of list entry.
 * @param position The &struct tea_list_entry to use as a loop cursor.
 * @param n Another &struct tea_list_entry to use as temporary storage.
 * @param head The head for your list.
 */
#define tea_list_for_each_safe(position, n, head)                              \
  for (position = (head)->next, n = position->next; position != (head);        \
       position = n, n = position->next)

/**
 * @brief Iterate over a list with an index counter.
 * @param index The index variable.
 * @param position The &struct tea_list_entry to use as a loop cursor.
 * @param head The head for your list.
 */
#define tea_list_for_each_indexed(index, position, head)                       \
  for (position = (head)->next, index = 0; position != (head);                 \
       position = position->next, index++)

/**
 * @brief Doubly linked list entry structure.
 *        Can be embedded in other structures to create linked lists.
 */
struct tea_list_entry_t {
  struct tea_list_entry_t *prev;
  struct tea_list_entry_t *next;
};

typedef struct tea_list_entry_t tea_list_entry_t;

/**
 * @brief Callback function type for list iteration.
 * @param index The current index in the iteration (starting from 0).
 * @param entry Pointer to the current list entry.
 * @param user_data User-provided data passed to the callback.
 * @return true to continue iteration, false to stop.
 */
typedef bool (*tea_list_callback_t)(unsigned long index,
                                    tea_list_entry_t *entry, void *user_data);

/**
 * @brief Initializes the head of a list.
 * @param head Pointer to the list head entry.
 */
void tea_list_init(tea_list_entry_t *head);

/**
 * @brief Checks if a list is empty.
 * @param head Pointer to the list head entry.
 * @return Non-zero if the list is empty, 0 otherwise.
 */
bool tea_list_empty(const tea_list_entry_t *head);

/**
 * @brief Adds a new entry to the front of the list.
 * @param head Pointer to the list head entry.
 * @param entry Pointer to the new entry to add.
 */
void tea_list_add_head(tea_list_entry_t *head, tea_list_entry_t *entry);

/**
 * @brief Adds a new entry to the back of the list.
 * @param head Pointer to the list head entry.
 * @param entry Pointer to the new entry to add.
 */
void tea_list_add_tail(tea_list_entry_t *head, tea_list_entry_t *entry);

/**
 * @brief Removes an entry from the list.
 * @param entry Pointer to the entry to remove.
 *        Note: entry is not freed, only removed from the list.
 */
void tea_list_remove(const tea_list_entry_t *entry);

/**
 * @brief Gets the first entry in the list.
 * @param head Pointer to the list head entry.
 * @return Pointer to the first entry, or NULL if the list is empty.
 */
tea_list_entry_t *tea_list_first(const tea_list_entry_t *head);

/**
 * @brief Gets the next entry in the list.
 * @param current Pointer to the current entry.
 * @param head Pointer to the list head entry.
 * @return Pointer to the next entry, or NULL if current is the last entry.
 */
tea_list_entry_t *tea_list_next(const tea_list_entry_t *current,
                                const tea_list_entry_t *head);

/**
 * @brief Gets the length (number of entries) in the list.
 * @param head Pointer to the list head entry.
 * @return The number of entries in the list (excluding the head).
 */
unsigned long tea_list_length(const tea_list_entry_t *head);

/**
 * @brief Iterate over a list using a callback function with index support.
 * @param head Pointer to the list head entry.
 * @param callback Callback function to call for each entry.
 * @param user_data User data to pass to the callback function.
 */
void tea_list_for_each_callback(const tea_list_entry_t *head,
                                tea_list_callback_t callback, void *user_data);
