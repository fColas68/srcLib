import lib.cards.K8055.K8055JNI;






public class Test_lib_cards_K8055_K8055JNI{

    static K8055JNI carte = new K8055JNI();


    public static void main(String args[])
    {
       carte.test();
       int res;
       res = carte.openDevice((long) 0);
       if (res==0)
       {
           int i = 0;
           long lec1;
           long lec2;
           do
           {
               lec2 = carte.readAnalogChannel(2);
               System.out.println(lec2);
               i++;
           } while (i < 300);

        carte.closeDevice();
       }


    }








}



