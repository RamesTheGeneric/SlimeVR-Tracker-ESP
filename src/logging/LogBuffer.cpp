#include "LogBuffer.h"

namespace SlimeVR::Logging {

bool LogBuffer::addLog(const char* message) {
	size_t len = strlen(message);
	
	// Need space for message + null terminator + length prefix (2 bytes)
	size_t required = len + 3;
	
	if (required > getAvailableSpace()) {
		// Buffer full, drop the message or overwrite oldest
		return false;
	}

	// Write length as 2-byte prefix (little endian)
	m_Buffer[m_WritePos] = (len >> 8) & 0xFF;
	m_WritePos = (m_WritePos + 1) % MAX_BUFFER_SIZE;
	m_Buffer[m_WritePos] = len & 0xFF;
	m_WritePos = (m_WritePos + 1) % MAX_BUFFER_SIZE;

	// Write message
	for (size_t i = 0; i < len; i++) {
		m_Buffer[m_WritePos] = message[i];
		m_WritePos = (m_WritePos + 1) % MAX_BUFFER_SIZE;
	}

	// Write null terminator
	m_Buffer[m_WritePos] = '\0';
	m_WritePos = (m_WritePos + 1) % MAX_BUFFER_SIZE;

	return true;
}

void LogBuffer::processCycle() {
	size_t bytesWritten = 0;

	while (m_WritePos != m_ReadPos && bytesWritten < MAX_BYTES_PER_CYCLE) {
		size_t msgLen = getNextMessageLength();
		
		if (msgLen == 0 || bytesWritten + msgLen > MAX_BYTES_PER_CYCLE) {
			// Message too large for this cycle or corrupted
			break;
		}

		// Skip length prefix
		m_ReadPos = (m_ReadPos + 2) % MAX_BUFFER_SIZE;

		// Read and print message
		for (size_t i = 0; i < msgLen; i++) {
			Serial.write(m_Buffer[m_ReadPos]);
			m_ReadPos = (m_ReadPos + 1) % MAX_BUFFER_SIZE;
		}

		// Skip null terminator
		m_ReadPos = (m_ReadPos + 1) % MAX_BUFFER_SIZE;

		bytesWritten += msgLen;
	}
}

size_t LogBuffer::getPendingBytes() const {
	if (m_WritePos >= m_ReadPos) {
		return m_WritePos - m_ReadPos;
	} else {
		return MAX_BUFFER_SIZE - m_ReadPos + m_WritePos;
	}
}

void LogBuffer::flushAll() {
	while (m_WritePos != m_ReadPos) {
		size_t msgLen = getNextMessageLength();
		
		if (msgLen == 0) {
			break;  // Corrupted buffer
		}

		// Skip length prefix
		m_ReadPos = (m_ReadPos + 2) % MAX_BUFFER_SIZE;

		// Read and print message
		for (size_t i = 0; i < msgLen; i++) {
			Serial.write(m_Buffer[m_ReadPos]);
			m_ReadPos = (m_ReadPos + 1) % MAX_BUFFER_SIZE;
		}

		// Skip null terminator
		m_ReadPos = (m_ReadPos + 1) % MAX_BUFFER_SIZE;
	}
}

size_t LogBuffer::getAvailableSpace() const {
	size_t used = getPendingBytes();
	return MAX_BUFFER_SIZE - used - 1;  // -1 to distinguish full from empty
}

size_t LogBuffer::getNextMessageLength() const {
	if (m_WritePos == m_ReadPos) {
		return 0;
	}

	size_t pos = m_ReadPos;
	uint8_t highByte = m_Buffer[pos];
	pos = (pos + 1) % MAX_BUFFER_SIZE;
	uint8_t lowByte = m_Buffer[pos];

	return (highByte << 8) | lowByte;
}

}  // namespace SlimeVR::Logging
