/**
 * @file device.cpp
 * @brief Лабораторная работа. Реализация классов Stream, Device, Mixer, Reactor с поддержкой weak_ptr.
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <gtest/gtest.h>

using namespace std;

int streamcounter = 0;
const float POSSIBLE_ERROR = 0.01;
const int MIXER_OUTPUTS = 1;

/**
 * @class Device
 * @brief Базовый класс технологического аппарата.
 */
class Device;

/**
 * @class Stream
 * @brief Класс материального потока.
 *
 * Хранит массовый расход, имя и ссылки на аппарат-источник и аппарат-получатель
 * с помощью weak_ptr<Device>.
 */
class Stream {
private:
    double mass_flow = 0.0;
    string name;

    weak_ptr<Device> fromDevice; ///< Из какого аппарата вытекает.
    weak_ptr<Device> toDevice;   ///< В какой аппарат втекает.

public:

    /// Создание потока с уникальным именем (s1, s2...)
    Stream(int s) {
        name = "s" + to_string(s);
    }

    /// Установка массового расхода
    void setMassFlow(double m) { mass_flow = m; }

    /// Получение массового расхода
    double getMassFlow() const { return mass_flow; }

    /// Получить имя потока
    string getName() const { return name; }

    /// Связать поток с аппаратом-источником
    void setFrom(shared_ptr<Device> dev) { fromDevice = dev; }

    /// Связать поток с аппаратом-получателем
    void setTo(shared_ptr<Device> dev) { toDevice = dev; }

    /// Получить weak_ptr на аппарат-источник
    weak_ptr<Device> getFromDevice() const { return fromDevice; }

    /// Получить weak_ptr на аппарат-получатель
    weak_ptr<Device> getToDevice() const { return toDevice; }
};

/**
 * @class Device
 * @brief Базовый класс технологического аппарата.
 */
class Device : public enable_shared_from_this<Device> {
protected:
    int inputAmount = 0;
    int outputAmount = 0;
    bool isCalculated = false;

public:
    vector<shared_ptr<Stream>> inputs;
    vector<shared_ptr<Stream>> outputs;

    Device() = default;

    /// Добавить входной поток
    void addInput(shared_ptr<Stream> s) {
        if (inputs.size() >= inputAmount)
            throw runtime_error("Превышен лимит входных потоков");
        inputs.push_back(s);
        s->setTo(shared_from_this()); // поток втекает в аппарат
    }

    /// Добавить выходной поток
    void addOutput(shared_ptr<Stream> s) {
        if (outputs.size() >= outputAmount)
            throw runtime_error("Превышен лимит выходных потоков");
        outputs.push_back(s);
        s->setFrom(shared_from_this()); // поток вытекает из аппарата
    }

    /// Проверка, был ли аппарат уже рассчитан
    bool isDeviceCalculated() const { return isCalculated; }

    /// Обновление выходных потоков — виртуальная функция
    virtual void updateOutputs() = 0;

    /// Получение имени аппарата (по адресу)
    virtual string getDeviceName() const {
        return "Device@" + to_string((uintptr_t)this);
    }
};

/**
 * @class Mixer
 * @brief Узел смешения: суммирует входные потоки.
 */
class Mixer : public Device {
private:
    int requiredInputs;

public:
    Mixer(int n) {
        inputAmount = n;
        outputAmount = MIXER_OUTPUTS;
        requiredInputs = n;
    }

    void addInput(shared_ptr<Stream> s) {
        if (inputs.size() >= requiredInputs)
            throw runtime_error("Too much inputs");
        inputs.push_back(s);
        s->setTo(shared_from_this());
    }

    void addOutput(shared_ptr<Stream> s) {
        if (outputs.size() >= MIXER_OUTPUTS)
            throw runtime_error("Too much outputs");
        outputs.push_back(s);
        s->setFrom(shared_from_this());
    }

    void updateOutputs() override {
        if (isCalculated)
            throw runtime_error("Device is already calculated");

        double sum = 0.0;
        for (auto &st : inputs)
            sum += st->getMassFlow();

        if (outputs.empty())
            throw runtime_error("Нет выходных потоков");

        outputs[0]->setMassFlow(sum);
        isCalculated = true;
    }
};

/**
 * @class Reactor
 * @brief Реактор: делит вход на 1 или 2 выхода.
 */
class Reactor : public Device {
public:
    Reactor(bool doubleOutput) {
        inputAmount = 1;
        outputAmount = doubleOutput ? 2 : 1;
    }

    void updateOutputs() override {
        if (isCalculated)
            throw runtime_error("Device is already calculated");

        double m = inputs[0]->getMassFlow();
        double part = m / outputAmount;

        for (auto &o : outputs)
            o->setMassFlow(part);

        isCalculated = true;
    }
};

//
// ------------------------- ТЕСТЫ -------------------------
//

TEST(StreamTests, FromToSetCorrectly) {
    auto dev = make_shared<Mixer>(2);
    auto s = make_shared<Stream>(1);

    s->setFrom(dev);
    s->setTo(dev);

    ASSERT_FALSE(s->getFromDevice().expired());
    ASSERT_FALSE(s->getToDevice().expired());
}

TEST(MixerTests, SumOfFlows) {
    streamcounter = 0;
    auto mix = make_shared<Mixer>(2);

    auto s1 = make_shared<Stream>(++streamcounter);
    auto s2 = make_shared<Stream>(++streamcounter);
    auto s3 = make_shared<Stream>(++streamcounter);

    s1->setMassFlow(10);
    s2->setMassFlow(5);

    mix->addInput(s1);
    mix->addInput(s2);
    mix->addOutput(s3);

    mix->updateOutputs();

    ASSERT_NEAR(s3->getMassFlow(), 15.0, POSSIBLE_ERROR);
}

TEST(DeviceTests, WrongInputAmount) {
    auto mix = make_shared<Mixer>(1);
    auto s1 = make_shared<Stream>(1);
    auto s2 = make_shared<Stream>(2);

    mix->addInput(s1);
    EXPECT_THROW(mix->addInput(s2), runtime_error);
}

TEST(DeviceTests, WrongOutputAmount) {
    auto mix = make_shared<Mixer>(2);
    auto s1 = make_shared<Stream>(1);
    auto s2 = make_shared<Stream>(2);

    mix->addOutput(s1);
    EXPECT_THROW(mix->addOutput(s2), runtime_error);
}

TEST(ReactorTests, SplitCorrectly) {
    auto r = make_shared<Reactor>(true);

    auto s1 = make_shared<Stream>(1);
    auto o1 = make_shared<Stream>(2);
    auto o2 = make_shared<Stream>(3);

    s1->setMassFlow(10);

    r->addInput(s1);
    r->addOutput(o1);
    r->addOutput(o2);

    r->updateOutputs();

    ASSERT_NEAR(o1->getMassFlow(), 5.0, POSSIBLE_ERROR);
    ASSERT_NEAR(o2->getMassFlow(), 5.0, POSSIBLE_ERROR);
}

//
// ------------------------- MAIN -------------------------
//

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
