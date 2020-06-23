/**
 ******************************************************************************
 * @file    Message.cpp
 * @author  ST / Central Labs
 * @version V1.0.0
 * @date    6 Nov 2015
 * @brief   NDef Message class implementation
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <cstdlib>
#include "NDefLib/Message.h"
#include "RecordType/EmptyRecord.h"
#include "RecordType/RecordText.h"
#include "RecordType/RecordAAR.h"
#include "RecordType/RecordMimeType.h"
#include "RecordType/RecordURI.h"

namespace NDefLib {

uint16_t Message::getByteLength() const {
	uint16_t lenght = 2; //length size

	if (mRecords.size() == 0)
		return lenght + EmptyRecord().getByteLength();

	std::vector<Record*>::const_iterator it = mRecords.begin();
	const std::vector<Record*>::const_iterator end = mRecords.end();

	for (; it != end; ++it) {
		lenght += (*it)->getByteLength();
	} //for

	return lenght;
} //getByteLenght

uint16_t Message::write(uint8_t *buffer) const {

	const uint16_t length = getByteLength() - 2;
	uint16_t offset = 0;
	buffer[offset++] = (uint8_t) ((length & 0xFF00) >> 8);
	buffer[offset++] = (uint8_t) ((length & 0x00FF));

	const uint32_t nRecord = mRecords.size();

	if (mRecords.size() == 0) {
		offset += EmptyRecord().write(buffer + offset);
		return offset;
	} //else

	for (uint32_t i = 0; i < nRecord; i++) {
		Record *r = mRecords[i];

		r->setAsMiddleRecord();
		if (i == 0)
			r->setAsFirstRecord();
		if (i == nRecord - 1)
			r->setAsLastRecord();

		offset += r->write(buffer + offset);
	} //for

	return offset;
} //write

void Message::parseMessage(const uint8_t * const rawNdefFile,
		const uint16_t length, Message *msg) {
	uint16_t offset = 0;
	Record *r;

	RecordHeader header;
	do {
		const uint8_t headerLenght = header.loadHeader(rawNdefFile + offset);
		r = RecordText::parse(header, rawNdefFile + offset + headerLenght);
		if (r == NULL)
			r = RecordAAR::parse(header, rawNdefFile + offset + headerLenght);
		if (r == NULL)
			r = RecordMimeType::parse(header,
					rawNdefFile + offset + headerLenght);
		if (r == NULL)
			r = RecordURI::parse(header, rawNdefFile + offset + headerLenght);

		offset += header.getRecordLength();
		msg->addRecord(r);
	} while (offset < length);

}

void Message::removeAndDeleteAllRecord(Message &msg){
    const uint32_t nRecords =msg.getNRecords();
	for(int i =0 ;i<nRecords ;i++){
        NDefLib::Record *r = msg[i];
        delete r;
    }//for
    msg.mRecords.clear();
}//removeAndDeleteAllRecord


} /* namespace NDefLib */
