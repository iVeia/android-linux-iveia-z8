/*
 * Copyright (C) 2012-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_sync.h
 *
 * Mali interface for Linux sync objects.
 */

#ifndef _MALI_SYNC_H_
#define _MALI_SYNC_H_

#if defined(CONFIG_SYNC) || defined(CONFIG_SYNC_FILE)

#include <linux/seq_file.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#include <linux/sync.h>
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
#include <sync.h>
#else
#include "mali_internal_sync.h"
#endif


#include "mali_osk.h"

struct mali_sync_flag;
struct mali_timeline;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
/**
 * Create a sync timeline.
 *
 * @param name Name of the sync timeline.
 * @return The new sync timeline if successful, NULL if not.
 */
struct sync_timeline *mali_sync_timeline_create(struct mali_timeline *timeline, const char *name);

/**
 * Creates a file descriptor representing the sync fence.  Will release sync fence if allocation of
 * file descriptor fails.
 *
 * @param sync_fence Sync fence.
 * @return File descriptor representing sync fence if successful, or -1 if not.
 */
s32 mali_sync_fence_fd_alloc(struct sync_fence *sync_fence);

/**
 * Merges two sync fences.  Both input sync fences will be released.
 *
 * @param sync_fence1 First sync fence.
 * @param sync_fence2 Second sync fence.
 * @return New sync fence that is the result of the merger if successful, or NULL if not.
 */
struct sync_fence *mali_sync_fence_merge(struct sync_fence *sync_fence1, struct sync_fence *sync_fence2);

/**
 * Create a sync fence that is already signaled.
 *
 * @param tl Sync timeline.
 * @return New signaled sync fence if successful, NULL if not.
 */
struct sync_fence *mali_sync_timeline_create_signaled_fence(struct sync_timeline *sync_tl);


/**
 * Create a sync flag.
 *
 * @param sync_tl Sync timeline.
 * @param point Point on Mali timeline.
 * @return New sync flag if successful, NULL if not.
 */
struct mali_sync_flag *mali_sync_flag_create(struct sync_timeline *sync_tl, u32 point);

/**
 * Create a sync fence attached to given sync flag.
 *
 * @param flag Sync flag.
 * @return New sync fence if successful, NULL if not.
 */
struct sync_fence *mali_sync_flag_create_fence(struct mali_sync_flag *flag);
#else
/**
 * Create a sync timeline.
 *
 * @param name Name of the sync timeline.
 * @return The new sync timeline if successful, NULL if not.
 */
struct mali_internal_sync_timeline *mali_sync_timeline_create(struct mali_timeline *timeline, const char *name);

/**
 * Creates a file descriptor representing the sync fence.  Will release sync fence if allocation of
 * file descriptor fails.
 *
 * @param sync_fence Sync fence.
 * @return File descriptor representing sync fence if successful, or -1 if not.
 */
s32 mali_sync_fence_fd_alloc(struct mali_internal_sync_fence *sync_fence);

/**
 * Merges two sync fences.  Both input sync fences will be released.
 *
 * @param sync_fence1 First sync fence.
 * @param sync_fence2 Second sync fence.
 * @return New sync fence that is the result of the merger if successful, or NULL if not.
 */
struct mali_internal_sync_fence *mali_sync_fence_merge(struct mali_internal_sync_fence *sync_fence1, struct mali_internal_sync_fence *sync_fence2);

/**
 * Create a sync fence that is already signaled.
 *
 * @param tl Sync timeline.
 * @return New signaled sync fence if successful, NULL if not.
 */
struct mali_internal_sync_fence *mali_sync_timeline_create_signaled_fence(struct mali_internal_sync_timeline *sync_tl);


/**
 * Create a sync flag.
 *
 * @param sync_tl Sync timeline.
 * @param point Point on Mali timeline.
 * @return New sync flag if successful, NULL if not.
 */
struct mali_sync_flag *mali_sync_flag_create(struct mali_internal_sync_timeline *sync_tl, u32 point);

/**
 * Create a sync fence attached to given sync flag.
 *
 * @param flag Sync flag.
 * @return New sync fence if successful, NULL if not.
 */
struct mali_internal_sync_fence *mali_sync_flag_create_fence(struct mali_sync_flag *flag);

#endif
/**
 * Grab sync flag reference.
 *
 * @param flag Sync flag.
 */
void mali_sync_flag_get(struct mali_sync_flag *flag);

/**
 * Release sync flag reference.  If this was the last reference, the sync flag will be freed.
 *
 * @param flag Sync flag.
 */
void mali_sync_flag_put(struct mali_sync_flag *flag);

/**
 * Signal sync flag.  All sync fences created from this flag will be signaled.
 *
 * @param flag Sync flag to signal.
 * @param error Negative error code, or 0 if no error.
 */
void mali_sync_flag_signal(struct mali_sync_flag *flag, int error);

#endif /* defined(CONFIG_SYNC) || defined(CONFIG_SYNC_FILE) */

#endif /* _MALI_SYNC_H_ */
