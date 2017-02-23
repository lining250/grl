// automatically generated, do not modify

package grl.flatbuffer;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class EulerRotationParams extends Table {
  public static EulerRotationParams getRootAsEulerRotationParams(ByteBuffer _bb) { return getRootAsEulerRotationParams(_bb, new EulerRotationParams()); }
  public static EulerRotationParams getRootAsEulerRotationParams(ByteBuffer _bb, EulerRotationParams obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public EulerRotationParams __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public double r1() { int o = __offset(4); return o != 0 ? bb.getDouble(o + bb_pos) : 0; }
  public double r2() { int o = __offset(6); return o != 0 ? bb.getDouble(o + bb_pos) : 0; }
  public double r3() { int o = __offset(8); return o != 0 ? bb.getDouble(o + bb_pos) : 0; }
  public byte eulerOrder() { int o = __offset(10); return o != 0 ? bb.get(o + bb_pos) : 0; }

  public static int createEulerRotationParams(FlatBufferBuilder builder,
      double r1,
      double r2,
      double r3,
      byte eulerOrder) {
    builder.startObject(4);
    EulerRotationParams.addR3(builder, r3);
    EulerRotationParams.addR2(builder, r2);
    EulerRotationParams.addR1(builder, r1);
    EulerRotationParams.addEulerOrder(builder, eulerOrder);
    return EulerRotationParams.endEulerRotationParams(builder);
  }

  public static void startEulerRotationParams(FlatBufferBuilder builder) { builder.startObject(4); }
  public static void addR1(FlatBufferBuilder builder, double r1) { builder.addDouble(0, r1, 0); }
  public static void addR2(FlatBufferBuilder builder, double r2) { builder.addDouble(1, r2, 0); }
  public static void addR3(FlatBufferBuilder builder, double r3) { builder.addDouble(2, r3, 0); }
  public static void addEulerOrder(FlatBufferBuilder builder, byte eulerOrder) { builder.addByte(3, eulerOrder, 0); }
  public static int endEulerRotationParams(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

