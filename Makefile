BIN_PATH := "D:\Workspace\.MIPT\KP\sysprog\RemoteCMD\x64\Debug\RemoteCMD.exe --svc -s --port 12345"

install_service:
	sc create RemoteCMDServer binPath= $(BIN_PATH)

delete_service:
	sc delete RemoteCMDServer

start_service:
	sc start RemoteCMDServer

stop_service:
	sc stop RemoteCMDServer

.PHONY: install_service delete_service start_service stop_service
