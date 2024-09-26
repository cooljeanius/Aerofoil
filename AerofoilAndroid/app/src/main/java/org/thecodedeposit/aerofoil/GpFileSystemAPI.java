package org.thecodedeposit.aerofoil;

public class GpFileSystemAPI
{
    public static native void nativePostSourceExportRequest(boolean cancelled, int fd, Object pfd);
}
