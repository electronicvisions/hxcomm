namespace hxcomm {

template <typename T>
DoubleBuffer<T>::DoubleBuffer(std::atomic<bool>& run) :
    m_data{new T(), new T()},
    m_read_available_count(0),
    m_write_position(0),
    m_read_position(0),
    m_mutex(),
    m_cv(),
    m_run(run)
{}

template <typename T>
DoubleBuffer<T>::DoubleBuffer(DoubleBuffer const& other) :
    m_data{new T(other.m_data[0]), new T(other.m_data[1])},
    m_read_available_count(other.m_read_available_count),
    m_write_position(other.m_write_position),
    m_read_position(other.read_position),
    m_mutex(other.m_mutex),
    m_cv(other.m_cv),
    m_run(other.m_run)
{}

template <typename T>
DoubleBuffer<T>& DoubleBuffer<T>::operator=(DoubleBuffer const& other)
{
	*(m_data[0]) = *(other.m_data[0]);
	*(m_data[1]) = *(other.m_data[1]);
	m_read_available_count = other.m_read_available_count;
	m_write_position = other.m_write_position;
	m_read_position = other.read_position;
	m_mutex = other.m_mutex;
	m_cv = other.m_cv;
	m_run = other.m_run;
}

template <typename T>
DoubleBuffer<T>::~DoubleBuffer()
{
	delete m_data[0];
	delete m_data[1];
}

template <typename T>
void DoubleBuffer<T>::notify()
{
	m_cv.notify_all();
}

template <typename T>
T* DoubleBuffer<T>::start_write()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this]() {
			return (m_read_available_count.load(std::memory_order_acquire) < 2 || !m_run);
		});
	}
	if (!m_run) {
		return nullptr;
	}
	return m_data[m_write_position];
}

template <typename T>
void DoubleBuffer<T>::stop_write()
{
	m_write_position = static_cast<size_t>(1) - m_write_position;
	m_read_available_count.fetch_add(1, std::memory_order_acq_rel);
	m_cv.notify_one();
}

template <typename T>
T const* DoubleBuffer<T>::start_read()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this]() {
			return (m_read_available_count.load(std::memory_order_acquire) > 0 || !m_run);
		});
	}
	if (!m_run) {
		return nullptr;
	}
	return m_data[m_read_position];
}

template <typename T>
void DoubleBuffer<T>::stop_read()
{
	m_read_position = static_cast<size_t>(1) - m_read_position;
	m_read_available_count.fetch_sub(1, std::memory_order_acq_rel);
	m_cv.notify_one();
}


template <typename SubpacketType, size_t MaxSize>
Packet<SubpacketType, MaxSize>::Packet() : data(), m_size(0)
{}

template <typename SubpacketType, size_t MaxSize>
void Packet<SubpacketType, MaxSize>::set_size(size_t const size)
{
	assert(size <= MaxSize);
	m_size = size;
}

template <typename SubpacketType, size_t MaxSize>
size_t Packet<SubpacketType, MaxSize>::get_size() const
{
	return m_size;
}

template <typename SubpacketType, size_t MaxSize>
typename Packet<SubpacketType, MaxSize>::array_type::const_iterator
Packet<SubpacketType, MaxSize>::cbegin() const
{
	return data.cbegin();
}

template <typename SubpacketType, size_t MaxSize>
typename Packet<SubpacketType, MaxSize>::array_type::const_iterator
Packet<SubpacketType, MaxSize>::cend() const
{
	return data.cbegin() + m_size;
}

} // namespace hxcomm
