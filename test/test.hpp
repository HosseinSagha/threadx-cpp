#pragma once

#include "flashSimulator.hpp"
#include "threadx-cpp/media.hpp"
#include "threadx-cpp/memoryPool.hpp"
#include "threadx-cpp/nandFlash.hpp"
#include "threadx-cpp/norFlash.hpp"
#include "threadx-cpp/queue.hpp"
#include "threadx-cpp/thread.hpp"
#include "threadx-cpp/tickTimer.hpp"
#include <cstddef>

inline constexpr ThreadX::Ulong thread0StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread1StackSize{2 * ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread2StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread3StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread4StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread5StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread6StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread7StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread8StackSize{2 * ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong thread9StackSize{ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong threadRamFileSystemStackSize{3 * ThreadX::minimumStackSize};
inline constexpr ThreadX::Ulong threadNorFileSystemStackSize{3 * ThreadX::minimumStackSize};
#if 0
inline constexpr ThreadX::Ulong threadNandFileSystemStackSize{8 * ThreadX::minimumStackSize};
#endif
inline constexpr ThreadX::Ulong queueSize{100};

inline constexpr ThreadX::Ulong threadMemPoolSize{
    ThreadX::minimumBytePoolSize({{thread0StackSize,
                                   thread1StackSize,
                                   thread2StackSize,
                                   thread3StackSize,
                                   thread4StackSize,
                                   thread5StackSize,
                                   thread6StackSize,
                                   thread7StackSize,
                                   thread8StackSize,
                                   thread9StackSize,
                                   threadRamFileSystemStackSize,
                                   threadNorFileSystemStackSize,
#if 0
       threadNandFileSystemStackSize,
#endif
                                   ThreadX::Ulong{sizeof(uint32_t) * queueSize}}})};

inline constexpr ThreadX::Ulong traceBufferSize{32'000};
inline constexpr size_t loggerStringReservedMemory{256};

class NorFlashSimulatorDriver
{
  public:
    LevelX::Error initialise();
    LevelX::Error read(const ThreadX::Ulong *flashAddress, const std::span<ThreadX::Ulong> destination);
    LevelX::Error write(ThreadX::Ulong *flashAddress, const std::span<const ThreadX::Ulong> source);
    LevelX::Error eraseBlock(const ThreadX::Ulong block, const ThreadX::Ulong eraseCount);
    LevelX::Error verifyErasedBlock(const ThreadX::Ulong block);
    LevelX::Error systemError(const ThreadX::Uint errorCode);
};

using ThreadPool = ThreadX::BytePool<threadMemPoolSize>;
using ThreadAllocator = ThreadX::Allocator<ThreadPool>;
using Thread = ThreadX::Thread<ThreadAllocator>;
using MsgQueue = ThreadX::Queue<uint32_t, queueSize, ThreadAllocator>;
using NorFlash = LevelX::NorFlash<4, NorFlashSimulatorDriver>;

#if 0
inline constexpr auto nandBlocks{4};
using NandFlash = LevelX::NandFlash<nandBlocks, 4, 512 + 16>;
#endif
// static inline std::vector<std::byte> m_ramStorage{2048};
inline constexpr auto norStorageSize{8 * 1024};
inline constexpr auto norBlocks{norStorageSize / sizeof(NorFlash::Block)};
extern NorFlash::Block norMem[norBlocks];

class Obj0
{
  public:
    void taskEntryFunction();

  private:
    uint32_t m_counter{};
};

class Obj1
{
  public:
    Obj1();

    void taskEntryFunction();

  private:
    void timerCallback(const uint32_t callbackID);

    ThreadX::TickTimer m_timer1;
    ThreadX::TickTimer m_timer2;
    uint32_t m_counter{};
    uint32_t m_messages_sent{};
    uint32_t timer1_counter{};
    uint32_t timer2_counter{};
};

class Obj2
{
  public:
    Obj2();

    void taskEntryFunction();
    void queueCallback(MsgQueue &queue);

  private:
    void timerCallback(const uint32_t callbackID);

    ThreadX::TickTimer m_timer;
    uint32_t m_counter{};
    uint32_t m_messages_received{};
    uint32_t timer_counter{};
};

class Obj3_4
{
  public:
    void taskEntryFunction();

  private:
    uint32_t m_counter{};
};

class Obj5
{
  public:
    void taskEntryFunction();

  private:
    uint32_t m_counter{};
};

class Obj6_7
{
  public:
    void taskEntryFunction();

  private:
    uint32_t m_counter{};
};

class ObjRamFileSystem
{
  public:
    ObjRamFileSystem(std::byte *driverInfoPtr);
    void taskEntryFunction();

  private:
    FileX::Media<> m_media;
};

class ObjNorFileSystem
{
  public:
    ObjNorFileSystem(NorFlashSimulatorDriver &norDriver);
    void taskEntryFunction();
    void mediaDriverCallback(FileX::Media<NorFlash::sectorSize()>::InternalDriver &media);

  private:
    NorFlash m_norFlash;
    FileX::Media<NorFlash::sectorSize()> m_media;
};

#if 0
class NandFlashDriver : public NandFlash
{
  public:
    using NandFlash::NandFlash;
    virtual ~NandFlashDriver() = default;

    LevelX::Error readCallback(const ThreadX::Ulong block, const ThreadX::Ulong page, ThreadX::Ulong *destination,
                               const ThreadX::Ulong words) final;
    LevelX::Error writeCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Ulong *source, ThreadX::Ulong words) final;
    LevelX::Error readPagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                    ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages) final;
    LevelX::Error writePagesCallback(ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *mainBuffer,
                                     ThreadX::Uchar *spareBuffer, ThreadX::Ulong pages) final;
    LevelX::Error copyPagesCallback(
        ThreadX::Ulong sourceBlock, ThreadX::Ulong sourcePage, ThreadX::Ulong destinationBlock,
        ThreadX::Ulong destinationPage, ThreadX::Ulong pages, ThreadX::Uchar *dataBuffer) final;
    LevelX::Error eraseBlockCallback(ThreadX::Ulong block, ThreadX::Ulong eraseCount) final;
    LevelX::Error verifyErasedBlockCallback(ThreadX::Ulong block) final;
    LevelX::Error verifyErasedPageCallback(ThreadX::Ulong block, ThreadX::Ulong page) final;
    LevelX::Error getBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar *badBlockFlag) final;
    LevelX::Error setBlockStatusCallback(ThreadX::Ulong block, ThreadX::Uchar badBlockFlag) final;
    LevelX::Error getExtraBytesCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *destination, ThreadX::Uint size) final;
    LevelX::Error setExtraBytesCallback(
        ThreadX::Ulong block, ThreadX::Ulong page, ThreadX::Uchar *source, ThreadX::Uint size) final;
};

class NandMedia : public FileX::Media<NandFlash::sectorSize()>
{
  public:
    NandMedia(NandFlashDriver &m_nandFlash);
    virtual ~NandMedia() = default;

    void driverCallback() final;

    NandFlashDriver &m_nandFlash;
};

class ObjNandFileSystem
  public:
    void taskEntryFunction();

  private:
    NandFlashDriver m_nandFlash;
    NandMedia m_media;
};
#endif

void runTestCode();
