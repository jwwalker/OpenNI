﻿using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UserID = System.Int32;

namespace OpenNI
{
	public class UserEventArgs : EventArgs
	{
		public UserEventArgs(UserID id)
		{
			this.id = id;
		}

		public UserID ID
		{
			get { return id; }
			set { id = value; }
		}

		private UserID id;
	}

	public class NewUserEventArgs : UserEventArgs
	{
		public NewUserEventArgs(UserID id) : base(id) { }
	}

	public class UserLostEventArgs : UserEventArgs
	{
		public UserLostEventArgs(UserID id) : base(id) { }
	}

    public class UserExitEventArgs : UserEventArgs
    {
        public UserExitEventArgs(UserID id) : base(id) { }
    }

    public class UserReEnterEventArgs : UserEventArgs
    {
        public UserReEnterEventArgs(UserID id) : base(id) { }
    }

	public class UserGenerator : Generator
    {
		public UserGenerator(Context context, IntPtr nodeHandle, bool addRef) : 
			base(context, nodeHandle, addRef)
        {
            this.internalNewUser = new SafeNativeMethods.XnUserHandler(this.InternalNewUser);
            this.internalLostUser = new SafeNativeMethods.XnUserHandler(this.InternalLostUser);
            this.internalUserExit = new SafeNativeMethods.XnUserHandler(this.InternalUserExit);
            this.internalUserReEnter = new SafeNativeMethods.XnUserHandler(this.InternalUserReEnter);
        }

        public UserGenerator(Context context, Query query, EnumerationErrors errors) :
			this(context, Create(context, query, errors), false)
        {
        }
        public UserGenerator(Context context, Query query)
            : this(context, query, null)
        {
        }
        public UserGenerator(Context context)
            : this(context, null, null)
        {
        }

        private static IntPtr Create(Context context, Query query, EnumerationErrors errors)
        {
            IntPtr handle;
            int status =
                SafeNativeMethods.xnCreateUserGenerator(context.InternalObject,
                                                        out handle,
                                                        query == null ? IntPtr.Zero : query.InternalObject,
                                                        errors == null ? IntPtr.Zero : errors.InternalObject);
            WrapperUtils.ThrowOnError(status);
            return handle;
        }

        public int NumberOfUsers
        {
			get
			{
				return SafeNativeMethods.xnGetNumberOfUsers(this.InternalObject);
			}
        }
        
        public UserID[] GetUsers()
        {
            ushort count = (ushort)NumberOfUsers;
            UserID[] users = new UserID[count];
            int status = SafeNativeMethods.xnGetUsers(this.InternalObject, users, ref count);
            WrapperUtils.ThrowOnError(status);
            return users;
        }
        
        public Point3D GetCoM(UserID id)
        {
            Point3D com = new Point3D();
            int status = SafeNativeMethods.xnGetUserCoM(this.InternalObject, id, out com);
            WrapperUtils.ThrowOnError(status);
            return com;
        }

        public SceneMetaData GetUserPixels(UserID id)
        {
            SceneMetaData smd = new SceneMetaData();
			using (IMarshaler marsh = smd.GetMarshaler(true))
			{
				int status = SafeNativeMethods.xnGetUserPixels(this.InternalObject, id, marsh.Native);
				WrapperUtils.ThrowOnError(status);
			}

            return smd;
        }

        public SkeletonCapability SkeletonCapability
        {
			get
			{
				return new SkeletonCapability(this);
			}
        }
		public PoseDetectionCapability PoseDetectionCapability
        {
			get
			{
				return new PoseDetectionCapability(this);
			}
        }

        #region New User
        private event EventHandler<NewUserEventArgs> newUserEvent;
		public event EventHandler<NewUserEventArgs> NewUser
        {
            add
            {
                if (this.newUserEvent == null)
                {
                    int status = SafeNativeMethods.xnRegisterUserCallbacks(this.InternalObject, this.internalNewUser, null, IntPtr.Zero, out newUserHandle);
                    WrapperUtils.ThrowOnError(status);
                }
                this.newUserEvent += value;
            }
            remove
            {
                this.newUserEvent -= value;

                if (this.newUserEvent == null)
                {
                    SafeNativeMethods.xnUnregisterUserCallbacks(this.InternalObject, this.newUserHandle);
                }
            }
        }
        private void InternalNewUser(IntPtr hNode, UserID id, IntPtr pCookie)
        {
			EventHandler<NewUserEventArgs> handlers = this.newUserEvent;
			if (handlers != null)
				handlers(this, new NewUserEventArgs(id));
        }
        private SafeNativeMethods.XnUserHandler internalNewUser;
        private IntPtr newUserHandle;
        #endregion

        #region Lost User
		private event EventHandler<UserLostEventArgs> lostUserEvent;
		public event EventHandler<UserLostEventArgs> LostUser
        {
            add
            {
                if (this.lostUserEvent == null)
                {
                    int status = SafeNativeMethods.xnRegisterUserCallbacks(this.InternalObject, null, this.internalLostUser, IntPtr.Zero, out lostUserHandle);
                    WrapperUtils.ThrowOnError(status);
                }
                this.lostUserEvent += value;
            }
            remove
            {
                this.lostUserEvent -= value;

                if (this.lostUserEvent == null)
                {
                    SafeNativeMethods.xnUnregisterUserCallbacks(this.InternalObject, this.lostUserHandle);
                }
            }
        }
        private void InternalLostUser(IntPtr hNode, UserID id, IntPtr pCookie)
        {
			EventHandler<UserLostEventArgs> handlers = this.lostUserEvent;
			if (handlers != null)
				handlers(this, new UserLostEventArgs(id));
        }
        private SafeNativeMethods.XnUserHandler internalLostUser;
        private IntPtr lostUserHandle;
        #endregion

        #region User Exit
        private event EventHandler<UserExitEventArgs> userExitEvent;
        public event EventHandler<UserExitEventArgs> UserExit
        {
            add
            {
                if (this.userExitEvent == null)
                {
                    int status = SafeNativeMethods.xnRegisterToUserExit(this.InternalObject, this.internalUserExit, IntPtr.Zero, out userExitHandle);
                    WrapperUtils.ThrowOnError(status);
                }
                this.userExitEvent += value;
            }
            remove
            {
                this.userExitEvent -= value;

                if (this.userExitEvent == null)
                {
                    SafeNativeMethods.xnUnregisterFromUserExit(this.InternalObject, this.userExitHandle);
                }
            }
        }
        private void InternalUserExit(IntPtr hNode, UserID id, IntPtr pCookie)
        {
            EventHandler<UserExitEventArgs> handlers = this.userExitEvent;
            if (handlers != null)
                handlers(this, new UserExitEventArgs(id));
        }
        private SafeNativeMethods.XnUserHandler internalUserExit;
        private IntPtr userExitHandle;
        #endregion

        #region User ReEnter
        private event EventHandler<UserReEnterEventArgs> userReEnterEvent;
        public event EventHandler<UserReEnterEventArgs> UserReEnter
        {
            add
            {
                if (this.userReEnterEvent == null)
                {
                    int status = SafeNativeMethods.xnRegisterToUserReEnter(this.InternalObject, this.internalUserReEnter, IntPtr.Zero, out userReEnterHandle);
                    WrapperUtils.ThrowOnError(status);
                }
                this.userReEnterEvent += value;
            }
            remove
            {
                this.userReEnterEvent -= value;

                if (this.userReEnterEvent == null)
                {
                    SafeNativeMethods.xnUnregisterFromUserReEnter(this.InternalObject, this.userReEnterHandle);
                }
            }
        }
        private void InternalUserReEnter(IntPtr hNode, UserID id, IntPtr pCookie)
        {
            EventHandler<UserReEnterEventArgs> handlers = this.userReEnterEvent;
            if (handlers != null)
                handlers(this, new UserReEnterEventArgs(id));
        }
        private SafeNativeMethods.XnUserHandler internalUserReEnter;
        private IntPtr userReEnterHandle;
        #endregion
    }
}
