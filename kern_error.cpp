/*
 * Copyright (c) 1999-2014 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* 
 *	File:	ubc_subr.c
 *	Author:	Umesh Vaishampayan [umeshv@apple.com]
 *		05-Aug-1999	umeshv	Created.
 *
 *	Functions related to Unified Buffer cache.
 *
 * Caller of UBC functions MUST have a valid reference on the vnode.
 *
 */ 

#include "kern_error.h"
/* 
 * This should be public but currently it is only used below so we
 * defer making that change.
 */
static int mach_to_bsd_errno(kern_return_t mach_err)
{
	switch (mach_err) {
	case KERN_SUCCESS:
		return 0;

	case KERN_INVALID_ADDRESS:
	case KERN_INVALID_ARGUMENT:
	case KERN_NOT_IN_SET:
	case KERN_INVALID_NAME:
	case KERN_INVALID_TASK:
	case KERN_INVALID_RIGHT:
	case KERN_INVALID_VALUE:
	case KERN_INVALID_CAPABILITY:
	case KERN_INVALID_HOST:
	case KERN_MEMORY_PRESENT:
	case KERN_INVALID_PROCESSOR_SET:
	case KERN_INVALID_POLICY:
	case KERN_ALREADY_WAITING:
	case KERN_DEFAULT_SET:
	case KERN_EXCEPTION_PROTECTED:
	case KERN_INVALID_LEDGER:
	case KERN_INVALID_MEMORY_CONTROL:
	case KERN_INVALID_SECURITY:
	case KERN_NOT_DEPRESSED:
	case KERN_LOCK_OWNED:
	case KERN_LOCK_OWNED_SELF:
		return EINVAL;

	case KERN_PROTECTION_FAILURE:
	case KERN_NOT_RECEIVER:
	case KERN_NO_ACCESS:
	case KERN_POLICY_STATIC:
		return EACCES;

	case KERN_NO_SPACE:
	case KERN_RESOURCE_SHORTAGE:
	case KERN_UREFS_OVERFLOW:
	case KERN_INVALID_OBJECT:
		return ENOMEM;

	case KERN_FAILURE:
		return EIO;

	case KERN_MEMORY_FAILURE:
	case KERN_POLICY_LIMIT:
	case KERN_CODESIGN_ERROR:
		return EPERM;

	case KERN_MEMORY_ERROR:
		return EBUSY;

	case KERN_ALREADY_IN_SET:
	case KERN_NAME_EXISTS:
	case KERN_RIGHT_EXISTS:
		return EEXIST;

	case KERN_ABORTED:
		return EINTR;

	case KERN_TERMINATED:
	case KERN_LOCK_SET_DESTROYED:
	case KERN_LOCK_UNSTABLE:
	case KERN_SEMAPHORE_DESTROYED:
		return ENOENT;

	case KERN_RPC_SERVER_TERMINATED:
		return ECONNRESET;

	case KERN_NOT_SUPPORTED:
		return ENOTSUP;

	case KERN_NODE_DOWN:
		return ENETDOWN;

	case KERN_NOT_WAITING:
		return ENOENT;

	case KERN_OPERATION_TIMED_OUT:
		return ETIMEDOUT;

	default:
		return -1 /* EIO */;
	}
}


#include "kern_error.h"
#include <string>

namespace {
	class private_error_category : public std::error_category {

		virtual std::error_condition default_error_condition( int code ) const noexcept;
		virtual std::string message( int condition ) const noexcept;
		virtual const char* name() const noexcept;
	};

	std::error_condition private_error_category::default_error_condition( int code ) const noexcept
	{
		int i = mach_to_bsd_errno(code);
		if (i >= 0) return std::error_condition(i, std::generic_category());
		return std::error_condition(code, *this);
	}

	const char* private_error_category::name() const noexcept
	{
		return "kern";
	}
	std::string private_error_category::message( int condition ) const noexcept
	{
		switch (condition) {
		case KERN_SUCCESS: return "KERN_SUCCESS";
		case KERN_INVALID_ADDRESS: return "KERN_INVALID_ADDRESS";
		case KERN_PROTECTION_FAILURE: return "KERN_PROTECTION_FAILURE";
		case KERN_NO_SPACE: return "KERN_NO_SPACE";
		case KERN_INVALID_ARGUMENT: return "KERN_INVALID_ARGUMENT";
		case KERN_FAILURE: return "KERN_FAILURE";
		case KERN_RESOURCE_SHORTAGE: return "KERN_RESOURCE_SHORTAGE";
		case KERN_NOT_RECEIVER: return "KERN_NOT_RECEIVER";
		case KERN_NO_ACCESS: return "KERN_NO_ACCESS";
		case KERN_MEMORY_FAILURE: return "KERN_MEMORY_FAILURE";
		case KERN_MEMORY_ERROR: return "KERN_MEMORY_ERROR";
		case KERN_ALREADY_IN_SET: return "KERN_ALREADY_IN_SET";
		case KERN_NOT_IN_SET: return "KERN_NOT_IN_SET";
		case KERN_NAME_EXISTS: return "KERN_NAME_EXISTS";
		case KERN_ABORTED: return "KERN_ABORTED";
		case KERN_INVALID_NAME: return "KERN_INVALID_NAME";
		case KERN_INVALID_TASK: return "KERN_INVALID_TASK";
		case KERN_INVALID_RIGHT: return "KERN_INVALID_RIGHT";
		case KERN_INVALID_VALUE: return "KERN_INVALID_VALUE";
		case KERN_UREFS_OVERFLOW: return "KERN_UREFS_OVERFLOW";
		case KERN_INVALID_CAPABILITY: return "KERN_INVALID_CAPABILITY";
		case KERN_RIGHT_EXISTS: return "KERN_RIGHT_EXISTS";
		case KERN_INVALID_HOST: return "KERN_INVALID_HOST";
		case KERN_MEMORY_PRESENT: return "KERN_MEMORY_PRESENT";
		case KERN_MEMORY_DATA_MOVED: return "KERN_MEMORY_DATA_MOVED";
		case KERN_MEMORY_RESTART_COPY: return "KERN_MEMORY_RESTART_COPY";
		case KERN_INVALID_PROCESSOR_SET: return "KERN_INVALID_PROCESSOR_SET";
		case KERN_POLICY_LIMIT: return "KERN_POLICY_LIMIT";
		case KERN_INVALID_POLICY: return "KERN_INVALID_POLICY";
		case KERN_INVALID_OBJECT: return "KERN_INVALID_OBJECT";
		case KERN_ALREADY_WAITING: return "KERN_ALREADY_WAITING";
		case KERN_DEFAULT_SET: return "KERN_DEFAULT_SET";
		case KERN_EXCEPTION_PROTECTED: return "KERN_EXCEPTION_PROTECTED";
		case KERN_INVALID_LEDGER: return "KERN_INVALID_LEDGER";
		case KERN_INVALID_MEMORY_CONTROL: return "KERN_INVALID_MEMORY_CONTROL";
		case KERN_INVALID_SECURITY: return "KERN_INVALID_SECURITY";
		case KERN_NOT_DEPRESSED: return "KERN_NOT_DEPRESSED";
		case KERN_TERMINATED: return "KERN_TERMINATED";
		case KERN_LOCK_SET_DESTROYED: return "KERN_LOCK_SET_DESTROYED";
		case KERN_LOCK_UNSTABLE: return "KERN_LOCK_UNSTABLE";
		case KERN_LOCK_OWNED: return "KERN_LOCK_OWNED";
		case KERN_LOCK_OWNED_SELF: return "KERN_LOCK_OWNED_SELF";
		case KERN_SEMAPHORE_DESTROYED: return "KERN_SEMAPHORE_DESTROYED";
		case KERN_RPC_SERVER_TERMINATED: return "KERN_RPC_SERVER_TERMINATED";
		case KERN_RPC_TERMINATE_ORPHAN: return "KERN_RPC_TERMINATE_ORPHAN";
		case KERN_RPC_CONTINUE_ORPHAN: return "KERN_RPC_CONTINUE_ORPHAN";
		case KERN_NOT_SUPPORTED: return "KERN_NOT_SUPPORTED";
		case KERN_NODE_DOWN: return "KERN_NODE_DOWN";
		case KERN_NOT_WAITING: return "KERN_NOT_WAITING";
		case KERN_OPERATION_TIMED_OUT: return "KERN_OPERATION_TIMED_OUT";
		case KERN_CODESIGN_ERROR: return "KERN_CODESIGN_ERROR";
		case KERN_POLICY_STATIC: return "KERN_POLICY_STATIC";
		case KERN_INSUFFICIENT_BUFFER_SIZE: return "KERN_INSUFFICIENT_BUFFER_SIZE";
		case KERN_RETURN_MAX: return "KERN_RETURN_MAX";
		default: return "";
		}
	}

}

std::error_category &kern_category()
{
	static private_error_category ec;
	return ec;	
}
