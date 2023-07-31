package lib.cards.K8055;
public class K8055JNI {
    static {
        // doit sappeler libK8055JNI.so
        System.loadLibrary("K8055JNI");
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        K8055JNI a = new K8055JNI();

        a.test();
        a.openDevice(0);
        long res;
        res = a.readAnalogChannel(1);
        System.out.println(res);
        res = a.readAnalogChannel(1);
        System.out.println(res);
        res = a.readAnalogChannel(1);
        System.out.println(res);
        res = a.readAnalogChannel(1);
        System.out.println(res);
        //a.closeDevice();

    }
    //==============================================================
    // NATIVES
    //==============================================================
    public native void test();
    public native int openDevice(long boardAddress);
    public native int closeDevice();
    public native long readAnalogChannel(long Channel); // 1 ou 2

}
