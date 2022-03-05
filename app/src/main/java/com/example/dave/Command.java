package com.example.dave;

import java.nio.ByteBuffer;

public class Command {
    public enum CommandType {
        MOVE,
        STOP,
        SET_MODE
    }
    public enum OperationMode {
        ANARCHY,
        REMOTE_CONTROL
    }
    public enum Direction {
        FORWARD, // 0
        LEFT,    // 1
        RIGHT,   // 2
        BACK,    // 3
    }

    private CommandType commandType;
    private byte opCode;
    private short value;

    public byte[] getBytes() {
        ByteBuffer buf = ByteBuffer.allocate(4);
        buf.put((byte) commandType.ordinal());
        buf.put(opCode);
        buf.putShort(value);
        return buf.array();
    }

    public Command(CommandType commandType, byte opCode, int value){
        this.commandType = commandType;
        this.opCode = opCode;
        this.value = (short) value;
    }

    public static Command CreateSetModeRemoteControlCommand(){
        return new Command(CommandType.SET_MODE, (byte) OperationMode.REMOTE_CONTROL.ordinal(), 0);
    }

    public static Command CreateSetModeAnarchyCommand() {
        return new Command(CommandType.SET_MODE, (byte) OperationMode.ANARCHY.ordinal(), 0);
    }

    public static Command CreateMoveCommand(Direction d){
        return new Command(CommandType.MOVE, (byte) d.ordinal(), 0);
    }

    public static Command CreateStopCommand(){
        return new Command(CommandType.STOP, (byte) 0, 0);
    }
}
