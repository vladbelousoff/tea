#include "tea_list.h"
#include <stddef.h>

void tea_list_init(tea_list_entry_t *head)
{
  head->prev = head;
  head->next = head;
}

bool tea_list_empty(const tea_list_entry_t *head)
{
  return head->next == head;
}

/**
 * @internal
 * @brief Inserts a new entry between two known consecutive entries.
 *        This is an internal function, list users should use
 *        tea_list_push_front() or tea_list_push_back().
 * @param _new Pointer to the new entry to be inserted.
 * @param prev Pointer to the entry that will precede the new entry.
 * @param next Pointer to the entry that will follow the new entry.
 */
static void _tea_list_insert(tea_list_entry_t *_new, tea_list_entry_t *prev,
                             tea_list_entry_t *next)
{
  next->prev = _new;
  _new->next = next;
  _new->prev = prev;
  prev->next = _new;
}

/**
 * @brief Adds a new entry to the front of the list.
 * @param head Pointer to the list head entry.
 * @param entry Pointer to the new entry to add.
 */
void tea_list_add_head(tea_list_entry_t *head, tea_list_entry_t *entry)
{
  _tea_list_insert(entry, head, head->next);
}

void tea_list_add_tail(tea_list_entry_t *head, tea_list_entry_t *entry)
{
  _tea_list_insert(entry, head->prev, head);
}

/**
 * @internal
 * @brief Removes an entry by connecting its neighbors.
 *        This is an internal function, list users should use tea_list_remove().
 * @param prev Pointer to the previous entry.
 * @param next Pointer to the next entry.
 */
static void _tea_list_remove(tea_list_entry_t *prev, tea_list_entry_t *next)
{
  next->prev = prev;
  prev->next = next;
}

void tea_list_remove(const tea_list_entry_t *entry)
{
  _tea_list_remove(entry->prev, entry->next);
}

tea_list_entry_t *tea_list_first(const tea_list_entry_t *head)
{
  if (tea_list_empty(head)) {
    return NULL;
  }

  return head->next;
}

tea_list_entry_t *tea_list_next(const tea_list_entry_t *current,
                                const tea_list_entry_t *head)
{
  if (current == NULL || current->next == head) {
    return NULL;
  }

  return current->next;
}

unsigned long tea_list_length(const tea_list_entry_t *head)
{
  unsigned long length = 0;

  tea_list_entry_t *current;
  tea_list_for_each(current, head)
  {
    length++;
  }

  return length;
}

void tea_list_for_each_callback(const tea_list_entry_t *head,
                                tea_list_callback_t callback, void *user_data)
{
  if (head == NULL || callback == NULL) {
    return;
  }

  unsigned long index = 0;
  tea_list_entry_t *current;

  tea_list_for_each(current, head)
  {
    if (!callback(index, current, user_data)) {
      break; // Stop iteration if callback returns false
    }
    index++;
  }
}
