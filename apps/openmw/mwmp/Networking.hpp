
namespace mwmp
{
    class Main
    {
    public:
        Main();
        ~Main();

        static void Create();
        static void Destroy();
        static void Frame(float dt);
        static void UpdateWorld(float dt);
    private:
        static Main *pMain;
    };
}
