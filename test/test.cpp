#include "test.hpp"
#include "flashSimulator.hpp"
#include "rttLogger.hpp"
#include "simulatorMediaDriver.hpp"
#include "threadx-cpp/eventFlags.hpp"
#include "threadx-cpp/file.hpp"
#include "threadx-cpp/mutex.hpp"
#include "threadx-cpp/norFlash.hpp"
#include "threadx-cpp/semaphore.hpp"
#include "threadx-cpp/trace.hpp"

using namespace std::chrono_literals;

#if 0
extern ThreadX::Native::LX_NAND_FLASH nand_flash;
extern ThreadX::Native::ULONG lx_memory_buffer[8224 / sizeof(ThreadX::Native::ULONG)];

namespace ThreadX::Native
{
extern "C" void _lx_nand_flash_simulator_erase_all();
extern "C" void _fx_nand_flash_simulator_driver(FX_MEDIA *media_ptr);
extern "C" UINT _lx_nand_flash_simulator_initialize(LX_NAND_FLASH *nand_flash);
} // namespace ThreadX::Native
#endif

enum class Error : size_t
{
    none = 0,
    nullPtrAccess,
    lengthError,
    invalidArgument,
    alreadyInitialised,
    allocationError,
    unexpectedValue,
    unknownError
};

struct Run8
{
    void operator()() const;
};

struct Run9
{
    void operator()() const;
};

class Device
{
  public:
    Device &operator=(const Device &) = delete;
    Device(const Device &) = delete;

    static Device &instance();
    ThreadPool m_memoryPool;
    ThreadAllocator m_threadAllocator;

    Obj0 m_object0;
    Thread m_thread0;

    Obj1 m_object1;
    Thread m_thread1;

    Obj2 m_object2;
    Thread m_thread2;

    Obj3_4 m_object3;
    Thread m_thread3;

    Obj3_4 m_object4;
    Thread m_thread4;

    Obj5 m_object5;
    Thread m_thread5;

    Obj6_7 m_object6;
    Thread m_thread6;

    Obj6_7 m_object7;
    Thread m_thread7;

    Thread m_thread8;
    Thread m_thread9;

    ObjRamFileSystem m_objRamFileSystem;
    Thread m_threadRamFileSystem;

    NorFlashSimulatorDriver m_norDriver;
    ObjNorFileSystem m_objNorFileSystem;
    Thread m_threadNorFileSystem;
#if 0
    ObjNandFileSystem m_objNandFileSystem;
    Thread m_threadNandFileSystem;

#endif

    ThreadX::BinarySemaphore m_semaphore;
    ThreadX::EventFlags m_eventFlags;
    MsgQueue m_queue;
#ifndef NDEBUG
    [[gnu::section(".trace")]] static inline ThreadX::Trace<traceBufferSize> trace{20};
#endif

  private:
    Device();
};

// replacing operator new and delete to use the thread allocator
void *operator new(std::size_t size)
{
    return Device::instance().m_threadAllocator.allocate(size);
}

void operator delete(void *ptr) noexcept
{
    Device::instance().m_threadAllocator.deallocate(reinterpret_cast<std::byte *>(ptr), 0);
}

void operator delete(void *ptr, std::size_t n) noexcept
{
    Device::instance().m_threadAllocator.deallocate(reinterpret_cast<std::byte *>(ptr), n);
}

void *operator new[](std::size_t size)
{
    return Device::instance().m_threadAllocator.allocate(size);
}

void operator delete[](void *ptr) noexcept
{
    Device::instance().m_threadAllocator.deallocate(reinterpret_cast<std::byte *>(ptr), 0);
}

void operator delete[](void *ptr, std::size_t n) noexcept
{
    Device::instance().m_threadAllocator.deallocate(reinterpret_cast<std::byte *>(ptr), n);
}

static void stackErrorCallback(Thread &thread)
{
    LOG_ERR("Stack Overflow in %s", thread.name().data());
}

void runTestCode()
{
    Device::instance();
    Thread::registerStackErrorNotifyCallback(stackErrorCallback);
    RttLogger::init(RttLogger::Type::debug);
}

struct PrintName
{
    void operator()(Thread &thread, ThreadX::ThreadNotifyCondition cond) const
    {
        if (cond == ThreadX::ThreadNotifyCondition::entry)
        {
            LOG_INFO("%s entered", thread.name().data());
        }
        else
        {
            LOG_INFO("%s exited", thread.name().data());
        }
    }
};

Device::Device()
    : m_memoryPool("byte pool"),
      m_threadAllocator(m_memoryPool),
      m_object0{},
      m_thread0(
          "thread 0", m_threadAllocator, [this]() { m_object0.taskEntryFunction(); }, thread0StackSize, PrintName(), 1),
      m_object1{},
      m_thread1(
          "thread 1",
          m_threadAllocator,
          [this]() { m_object1.taskEntryFunction(); },
          thread1StackSize,
          PrintName(),
          ThreadX::defaultPriority,
          ThreadX::defaultPriority,
          4),
      m_object2{},
      m_thread2(
          "thread 2",
          m_threadAllocator,
          [this]() { m_object2.taskEntryFunction(); },
          thread2StackSize,
          PrintName(),
          ThreadX::defaultPriority,
          ThreadX::defaultPriority,
          4),
      m_object3{},
      m_thread3(
          "thread 3", m_threadAllocator, [this]() { m_object3.taskEntryFunction(); }, thread3StackSize, PrintName(), 8),
      m_object4{},
      m_thread4(
          "thread 4", m_threadAllocator, [this]() { m_object4.taskEntryFunction(); }, thread4StackSize, PrintName(), 8),
      m_object5{},
      m_thread5(
          "thread 5", m_threadAllocator, [this]() { m_object5.taskEntryFunction(); }, thread5StackSize, PrintName(), 4),
      m_object6{},
      m_thread6(
          "thread 6", m_threadAllocator, [this]() { m_object6.taskEntryFunction(); }, thread6StackSize, PrintName(), 8),
      m_object7{},
      m_thread7(
          "thread 7", m_threadAllocator, [this]() { m_object7.taskEntryFunction(); }, thread7StackSize, PrintName(), 8),
      m_thread8("thread 8", m_threadAllocator, Run8(), thread8StackSize, PrintName()),
      m_thread9("thread 9", m_threadAllocator, Run9(), thread9StackSize),
      m_objRamFileSystem(ramMem),
      m_threadRamFileSystem(
          "thread ram FS",
          m_threadAllocator,
          [this]() { m_objRamFileSystem.taskEntryFunction(); },
          threadRamFileSystemStackSize,
          PrintName()),
      m_norDriver(),
      m_objNorFileSystem(m_norDriver),
      m_threadNorFileSystem(
          "thread nor FS",
          m_threadAllocator,
          [this]() { m_objNorFileSystem.taskEntryFunction(); },
          threadNorFileSystemStackSize,
          PrintName()),
#if 0
      m_objNandFileSystem(),
      m_threadNandFileSystem("thread nand FS", m_threadAllocator, [this]() { m_objNandFileSystem.taskEntryFunction(); }, threadNandFileSystemStackSize, PrintName()),
#endif
      m_semaphore("semaphore 1", 1),
      m_eventFlags("event flags 1"),
      m_queue("queue 1", m_threadAllocator, [this](auto &queue) { m_object2.queueCallback(queue); })
{
}

Device &Device::instance()
{
    static Device device;
    return device;
}

void Obj0::taskEntryFunction()
{
    using namespace std::chrono_literals;

    auto &dev{Device::instance()};
    auto eventName{dev.m_eventFlags.name().data()};
    /* This thread simply sits in while-forever-sleep loop.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;

        /* Sleep for 10 ticks.  */
        ThreadX::ThisThread::sleepFor(100ms);

        /* Set event flag 0 to wakeup thread 5. */
        if (ThreadX::Error error{dev.m_eventFlags.set(0x1)}; error != ThreadX::Error::success)
        {
            LOG_ERR("%s ThreadX error %u!", eventName, error);
            break;
        }

        LOG_INFO("%s flag 0 set.", eventName);
    }
}

Obj1::Obj1()
    : m_timer1("timer1", 500ms, [this](auto id) { timerCallback(id); }),
      m_timer2("timer2", 1s, [this](auto id) { timerCallback(id); })
{
}

void Obj1::taskEntryFunction()
{
    auto &dev{Device::instance()};
    /* This thread simply sends messages to a queue shared by thread 2.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;
        /* Send message to queue 0.  */
        if (ThreadX::Error error{dev.m_queue.send(m_messages_sent)}; error != ThreadX::Error::success)
        {
            LOG_ERR("%s ThreadX error %u!", dev.m_queue.name().data(), error);
            break;
        }

        LOG_DBG("Queue message sent: %u", m_messages_sent);
        /* Increment the message sent.  */
        m_messages_sent++;
        LOG_INFO("No. of message sent: %u", m_messages_sent);
        LOG_INFO("Timer1 callback counter: %u", timer1_counter);
        LOG_INFO("Timer2 callback counter: %u", timer2_counter);

        ThreadX::ThisThread::sleepFor(100ms);
    }
}

void Obj1::timerCallback(const uint32_t id)
{
    // timer is handled in interrupt (TX_TIMER_PROCESS_IN_ISR is enabled). So cannot call logger from here because it
    // cannot lock the mutex and returns with error.
    if (id == m_timer1.id())
    {
        ++timer1_counter;
    }
    else if (id == m_timer2.id())
    {
        ++timer2_counter;
    }
}

Obj2::Obj2()
    : m_timer("timer3", 2s, [this](auto id) { timerCallback(id); })
{
}

void Obj2::taskEntryFunction()
{
    auto &dev{Device::instance()};
    auto queueName{dev.m_queue.name().data()};
    /* This thread retrieves messages placed on the queue by thread 1.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;
        /* Retrieve a message from the queue.  */
        auto received_message{dev.m_queue.receive()};
        if (received_message)
        { /* Check completion status and make sure the message is what we expected.  */
            if (*received_message != m_messages_received)
            {
                LOG_ERR("%s recieved message %u!", ThreadX::ThisThread::name().data(), Error::unexpectedValue);
                break;
            }
        }
        else
        {
            LOG_ERR("%s ThreadX error %u!", queueName, received_message.error());
            break;
        }

        /* Otherwise, all is okay.  Increment the received message count.  */
        m_messages_received++;
        LOG_DBG("%s message received: %u", queueName, *received_message);
        LOG_INFO("No. of message received: %u", m_messages_received);
        LOG_INFO("Timer3 callback counter: %u", timer_counter);
    }
}

void Obj2::timerCallback(const uint32_t callbackID)
{
    // timer is handled in interrupt (TX_TIMER_PROCESS_IN_ISR is enabled). So cannot call logger from here because it
    // cannot lock the mutex and returns with error.
    if (callbackID == m_timer.id())
    {
        ++timer_counter;
    }
}

void Obj2::queueCallback(MsgQueue &queue)
{
    LOG_INFO("%s message callback called.", queue.name().data());
}

void Obj3_4::taskEntryFunction()
{
    auto &dev{Device::instance()};
    auto semaphoreName{dev.m_semaphore.name().data()};
    ThreadX::Error error{};

    /* This function is executed from thread 3 and thread 4.  As the loop
       below shows, these function compete for ownership of semaphore_0.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;
        /* Get the semaphore with suspension.  */
        if (error = dev.m_semaphore.acquire(); error != ThreadX::Error::success)
        {
            LOG_ERR("%s ThreadX error %u!", semaphoreName, error);
            break;
        }

        LOG_INFO("%s acquired.", semaphoreName);
        /* Sleep for 2 ticks to hold the semaphore.  */
        ThreadX::ThisThread::sleepFor(100ms);
        /* Release the semaphore.  */
        if (error = dev.m_semaphore.release(); error != ThreadX::Error::success)
        {
            LOG_ERR("%s ThreadX error %u!", semaphoreName, error);
            break;
        }

        LOG_INFO("%s released.", semaphoreName);
    }
}

void Obj5::taskEntryFunction()
{
    const auto threadName{ThreadX::ThisThread::name().data()};
    /* This thread simply waits for an event in a forever loop.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;
        /* Wait for event flag 0.  */
        if (auto actual_flags{Device::instance().m_eventFlags.waitAll(0x1)})
        {
            if (*actual_flags != 0x1)
            {
                LOG_ERR("%s error %u!", threadName, Error::unexpectedValue);
                break;
            }
        }
        else
        {
            LOG_ERR("%s ThreadX error %u!", threadName, actual_flags.error());
            break;
        }

        LOG_INFO("%s flag 0 got.", Device::instance().m_eventFlags.name().data());
    }
}

void Obj6_7::taskEntryFunction()
{
    ThreadX::Error error{};
    ThreadX::Mutex mutex{};
    /* This function is executed from thread 6 and thread 7.  As the loop
       below shows, these function compete for ownership of mutex_0.  */
    while (1)
    {
        /* Increment the thread counter.  */
        m_counter++;
        /* Get the mutex with suspension.  */
        if (error = mutex.lock(); error != ThreadX::Error::success)
        {
            break;
        }

        LOG_INFO("mutex Locked.");
        /* Get the mutex again with suspension.  This shows
           that an owning thread may retrieve the mutex it
           owns multiple times.  */
        if (error = mutex.lock(); error != ThreadX::Error::success)
        {
            break;
        }

        LOG_INFO("mutex Locked.");
        /* Sleep for 2 ticks to hold the mutex.  */
        ThreadX::ThisThread::sleepFor(100ms);
        /* Release the mutex.  */
        if (error = mutex.unlock(); error != ThreadX::Error::success)
        {
            break;
        }

        LOG_INFO("mutex unlocked.");
        /* Release the mutex again.  This will actually
           release ownership since it was obtained twice.  */
        if (error = mutex.unlock(); error != ThreadX::Error::success)
        {
            break;
        }

        LOG_INFO("mutex unlocked.");
    }

    LOG_ERR("%s ThreadX error %u!", ThreadX::ThisThread::name().data(), error);
}

void Run8::operator()() const
{
    auto error = ThreadX::ThisThread::sleepFor(3s);
    if (error != ThreadX::Error::success)
    {
        LOG_ERR("%s ThreadX error %u!", ThreadX::ThisThread::name().data(), error);
    }
}

void Run9::operator()() const
{
    auto &dev{Device::instance()};

    while (1)
    {
        dev.m_thread8.join();
        LOG_INFO("%s joined to %s.", dev.m_thread8.name().data(), ThreadX::ThisThread::name().data());

        if (auto error{dev.m_thread8.restart()}; error != ThreadX::Error::success)
        {
            LOG_ERR("%s ThreadX error %u!", ThreadX::ThisThread::name().data(), error);
            break;
        }
    }
}

ObjRamFileSystem::ObjRamFileSystem(std::byte *driverInfoPtr)
    : m_media(ramMediaDriver, driverInfoPtr)
{
}

void ObjRamFileSystem::taskEntryFunction()
{
    FileX::Error error{m_media.open("ram media")};

    do
    {
        if (error == FileX::Error::bootError)
        {
            if (error = m_media.format("ram disk", 20 * 512); error != FileX::Error::success)
            {
                break;
            }

            if (error = m_media.open("ram media"); error != FileX::Error::success)
            {
                break;
            }
        }
        else if (error != FileX::Error::success)
        {
            break;
        }

        if (error = m_media.createFile("my file.txt"); error != FileX::Error::success)
        {
            break;
        }

        while (true)
        {
            FileX::File file("my file.txt", m_media, FileX::OpenOption::write);

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.write(" ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            std::byte localBuffer[28];
            if (auto actual{file.read(localBuffer)})
            {
                if (*actual != 28)
                {
                    LOG_ERR("Error reading file (%s).", ThreadX::ThisThread::name().data());
                    return;
                }
                else
                {
                    LOG_INFO("Success reading file (%s).", ThreadX::ThisThread::name().data());
                }
            }
            else
            {
                break;
            }

            ThreadX::ThisThread::sleepFor(1s);
        };
    } while (0);

    LOG_ERR("%s error %X!", ThreadX::ThisThread::name().data(), error);
}

void ObjNorFileSystem::mediaDriverCallback(FileX::Media<NorFlash::sectorSize()>::InternalDriver &media)
{
    norFlashSimulatorMediaDriver(media, m_norFlash);
}

ObjNorFileSystem::ObjNorFileSystem(NorFlashSimulatorDriver &norDriver)
    : m_norFlash(norDriver, sizeof(norMem), reinterpret_cast<ThreadX::Ulong>(norMem)),
      m_media{[this](auto &media) { mediaDriverCallback(media); }}
{
}

void ObjNorFileSystem::taskEntryFunction()
{
    FileX::Error error{m_media.open("nor media")};
    m_media.setFileSystemTime();
    do
    {
        if (error != FileX::Error::success)
        {
            norFlashSimulatorEraseAll();

            if (error = m_media.format("nor disk", m_norFlash.mediaFormatSize()); error != FileX::Error::success)
            {
                break;
            }

            if (error = m_media.open("nor media"); error != FileX::Error::success)
            {
                break;
            }
        }

        if (error = m_media.createFile("my file.txt"); error != FileX::Error::success)
        {
            break;
        }

        while (true)
        {
            FileX::File file("my file.txt", m_media, FileX::OpenOption::write);

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.write(" ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            std::byte localBuffer[28];
            if (auto actual{file.read(localBuffer)})
            {
                if (*actual != 28)
                {
                    LOG_ERR("Error reading file (%s).", ThreadX::ThisThread::name().data());
                    return;
                }
                else
                {
                    LOG_INFO("Success reading file (%s).", ThreadX::ThisThread::name().data());
                }
            }
            else
            {
                break;
            }

            ThreadX::ThisThread::sleepFor(1s);
        };
    } while (0);

    LOG_ERR("%s error 0x%X!", ThreadX::ThisThread::name().data(), error);
}

#if 0
NandMedia::NandMedia(NandFlashDriver &nandFlash) : m_nandFlash{nandFlash}
{
}

void NandMedia::driverCallback()
{
    nandFlashSimulatorMediaDriver(*this);
}

LevelX::Error NandFlashDriver::readCallback(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *destination, ThreadX::Ulong words)
{
    return nandFlashSimulatorRead(this, block, page, destination, words);
}

LevelX::Error NandFlashDriver::writeCallback(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source, ThreadX::Ulong words)
{
    return nandFlashSimulatorWrite(this, block, page, source, words);
}

LevelX::Error NandFlashDriver::readPagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                                 ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages)
{
    return nandFlashSimulatorPagesRead(this, block, page, mainBuffer, spareBuffer, pages);
}

LevelX::Error NandFlashDriver::writePagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                                  ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages)
{
    return nandFlashSimulatorPagesWrite(this, block, page, mainBuffer, spareBuffer, pages);
}

LevelX::Error NandFlashDriver::copyPagesCallback(
    ThreadX::Ulong sourceBlock, ThreadX::Ulong sourcePage, ThreadX::Ulong destinationBlock,
    ThreadX::Ulong destinationPage, ThreadX::Ulong pages, ThreadX::Uchar *dataBuffer)
{
    return nandFlashSimulatorPagesCopy(
        this, sourceBlock, sourcePage, destinationBlock, destinationPage, pages, dataBuffer);
}

LevelX::Error NandFlashDriver::eraseBlockCallback(ThreadX::Ulong block, ThreadX::Ulong eraseCount)
{
    return nandFlashSimulatorBlockErase(block, eraseCount);
}

LevelX::Error NandFlashDriver::verifyErasedBlockCallback(ThreadX::Ulong block)
{
    return nandFlashSimulatorBlockErasedVerify(block);
}

LevelX::Error NandFlashDriver::verifyErasedPageCallback(ThreadX::Ulong block, ThreadX::Ulong page)
{
    return nandFlashSimulatorPageErasedVerify(block, page);
}

LevelX::Error NandFlashDriver::getBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar *badBlockFlag)
{
    return nandFlashSimulatorBlockStatusGet(block, badBlockFlag);
}

LevelX::Error NandFlashDriver::setBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar badBlockFlag)
{
    return nandFlashSimulatorBlockStatusSet(block, badBlockFlag);
}

LevelX::Error NandFlashDriver::getExtraBytesCallback(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size)
{
    return nandFlashSimulatorExtraBytesGet(block, page, destination, size);
}

LevelX::Error NandFlashDriver::setExtraBytesCallback(
    ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size)
{
    return nandFlashSimulatorExtraBytesSet(block, page, source, size);
}

ObjNandFileSystem::ObjNandFileSystem(): m_nandFlash(LevelX::NandSpareDataInfo{4, 4, 2, 2}),
      m_media{m_nandFlash}
{
}

ThreadX::Native::FX_MEDIA nand_disk;
ThreadX::Native::FX_FILE my_file;
ThreadX::Native::FX_FILE my_file1;
unsigned char media_memory[4096];

void ObjNandFileSystem::taskEntryFunction()
{
#if 1
    FileX::Error error{m_media.open("nand media")};

    do
    {
        if (error != FileX::Error::success)
        {
            nandFlashSimulatorEraseAll();

            if (error = m_media.format("nand disk", m_nandFlash.mediaFormatSize()); error != FileX::Error::success)
            {
                break;
            }

            if (error = m_media.open("nand media"); error != FileX::Error::success)
            {
                break;
            }
        }

        if (error = m_media.createFile("my file.txt"); error != FileX::Error::success)
        {
            break;
        }

        while (true)
        {
            FileX::File file("my file.txt", m_media, FileX::OpenOption::write);

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.write(" ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"); error != FileX::Error::success)
            {
                break;
            }

            if (error = file.seek(0); error != FileX::Error::success)
            {
                break;
            }

            std::byte localBuffer[28];
            ThreadX::Uint actual;
            if (std::tie(error, actual) = file.read(localBuffer); error != FileX::Error::success)
            {
                break;
            }

            if (actual != 28)
            {
                LOG_ERR("Error reading file (%s).", name().data());
                return;
            }
            else
            {
                LOG_INFO("Success reading file (%s).", name().data());
            }

            LOG_INFO("%s max stack used: %u%%", name().data(), stackInfo().maxUsedPercent);

            ThreadX::ThisThread::sleepFor(1s);
        };
    } while (0);

    LOG_ERR("%s error 0x%X!", name().data(), error);
#endif
#if 0
    ThreadX::Native::UINT status;
    ThreadX::Native::ULONG actual;
    ThreadX::Native::CHAR local_buffer[30];

    /* Erase the simulated NAND flash.  */
    ThreadX::Native::_lx_nand_flash_simulator_erase_all();

    /* Format the NAND disk - the memory for the NAND flash disk is setup in 
       the NAND simulator. Note that for best performance, the format of the
       NAND flash should be less than one full NAND flash block of sectors.  */
    ThreadX::Native::fx_media_format(
        &nand_disk,
        ThreadX::Native::_fx_nand_flash_simulator_driver, // Driver entry
        nullptr,                                          // Unused
        media_memory,                                     // Media buffer pointer
        sizeof(media_memory),                             // Media buffer size
        const_cast<char *>("MY_NAND_DISK"),               // Volume Name
        1,                                                // Number of FATs
        32,                                               // Directory Entries
        0,                                                // Hidden sectors
        15,                                               // Total sectors
        2048,                                             // Sector size
        1,                                                // Sectors per cluster
        1,                                                // Heads
        1);                                               // Sectors per track
    ThreadX::Native::_lx_nand_flash_format(
        &nand_flash, const_cast<char *>("asd"), ThreadX::Native::_lx_nand_flash_simulator_initialize, lx_memory_buffer,
        sizeof(lx_memory_buffer));
    /* Loop to repeat the demo over and over!  */
    do
    {

        /* Open the NAND disk.  */
        status = ThreadX::Native::_fxe_media_open(
            std::addressof(nand_disk), const_cast<char *>("NAND DISK"),
            ThreadX::Native::_fx_nand_flash_simulator_driver, nullptr, media_memory, sizeof(media_memory),
            sizeof(ThreadX::Native::FX_MEDIA));

        /* Check the media open status.  */
        if (status != FX_SUCCESS)
        {

            /* Error, break the loop!  */
            break;
        }

        /* Create a file called TEST.TXT in the root directory.  */
        status = ThreadX::Native::fx_file_create(&nand_disk, const_cast<char *>("TEST.TXT"));

        /* Check the create status.  */
        if (status != FX_SUCCESS)
        {

            /* Check for an already created status. This is expected on the
               second pass of this loop!  */
            if (status != FX_ALREADY_CREATED)
            {

                /* Create error, break the loop.  */
                break;
            }
        }

        /* Open the test file.  */
        status = ThreadX::Native::_fxe_file_open(
            &nand_disk, &my_file, const_cast<char *>("TEST.TXT"), FX_OPEN_FOR_WRITE, sizeof(ThreadX::Native::FX_FILE));

        /* Check the file open status.  */
        if (status != FX_SUCCESS)
        {

            /* Error opening file, break the loop.  */
            break;
        }

        /* Seek to the beginning of the test file.  */
        status = ThreadX::Native::fx_file_seek(&my_file, 0);


        if (status != FX_SUCCESS)
        {

            /* Error performing file seek, break the loop.  */
            break;
        }

        /* Write a string to the test file.  */
        status = ThreadX::Native::fx_file_write(&my_file, const_cast<char *>(" ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"), 28);

        /* Check the file write status.  */
        if (status != FX_SUCCESS)
        {

            /* Error writing to a file, break the loop.  */
            break;
        }

        /* Seek to the beginning of the test file.  */
        status = ThreadX::Native::fx_file_seek(&my_file, 0);

        /* Check the file seek status.  */
        if (status != FX_SUCCESS)
        {

            /* Error performing file seek, break the loop.  */
            break;
        }

        /* Read the first 28 bytes of the test file.  */
        status = ThreadX::Native::fx_file_read(&my_file, local_buffer, 28, &actual);

        /* Check the file read status.  */
        if ((status != FX_SUCCESS) or (actual != 28))
        {

            /* Error reading file, break the loop.  */
            break;
        }

        /* Close the test file.  */
        status = ThreadX::Native::fx_file_close(&my_file);

        /* Check the file close status.  */
        if (status != FX_SUCCESS)
        {

            /* Error closing the file, break the loop.  */
            break;
        }

        /* Delete the file.  */
        status = ThreadX::Native::fx_file_delete(&nand_disk, const_cast<char *>("TEST.TXT"));

        /* Check the file delete status.  */
        if (status != FX_SUCCESS)
        {
            /* Error deleting the file, break the loop.  */
            break;
        }

        /* Close the media.  */
        status = ThreadX::Native::fx_media_close(&nand_disk);

        /* Check the media close status.  */
        if (status != FX_SUCCESS)
        {

            /* Error closing the media, break the loop.  */
            break;
        }

        /* Increment the thread counter, which represents the number
           of successful passes through this loop.  */
    } while (1);

    /* If we get here the FileX test failed!  */
    return;
#endif
}
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
