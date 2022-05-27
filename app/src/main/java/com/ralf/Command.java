package com.ralf;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Calendar;
import java.util.Date;

public class Command {
    public enum CommandType {
        MOVE,
        SET_MODE,
        SET_ALARM,
        DISABLE_ALARM
    }
    public enum OperationMode {
        ANARCHY,
        REMOTE_CONTROL
    }
    public enum Direction {
        FORWARD, // 0
        LEFT,    // 1
        RIGHT,   // 2
        REVERSE, // 3
        STOP    // 4
    }

    private CommandType commandType;
    private byte opCode;
    private short value;

    public short getValue() {
        return value;
    }

    public byte[] getBytes() {
        ByteBuffer buf = ByteBuffer.allocate(4);
        buf.order(ByteOrder.LITTLE_ENDIAN);
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
        return new Command(CommandType.MOVE, (byte) Direction.STOP.ordinal(), 0);
    }

    public static Command CreateDisableAlarmCommand(){ return new Command(CommandType.DISABLE_ALARM, (byte) 0, 0); }

    private static Date getTimeDeltaMilliseconds(int hours, int minutes){
        Calendar cal = Calendar.getInstance();
        Date now = cal.getTime();

        cal.set(Calendar.HOUR_OF_DAY, hours);
        cal.set(Calendar.MINUTE, minutes);

        Date date = cal.getTime();
        if (date.before(now)) {
            cal.add(Calendar.DATE, 1);
            date = cal.getTime();
        }
        long diffInMs = date.getTime() - now.getTime();
        return new Date(diffInMs);
    }

    private static short getTimeDeltaMinutes(int hours, int minutes){
        long diffInMs =  getTimeDeltaMilliseconds(hours, minutes).getTime();
        return  (short) (diffInMs / (60 * 1000));
    }

    public static Command CreateSetAlarmCommand(int hours, int minutes){
        short timeDeltaMinutes = getTimeDeltaMinutes(hours, minutes);
        return new Command(CommandType.SET_ALARM, (byte) 0, (int) timeDeltaMinutes);
    }
}
