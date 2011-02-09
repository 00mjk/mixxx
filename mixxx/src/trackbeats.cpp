#include <QtDebug>

#include "defs.h"
#include "trackinfoobject.h"
#include "trackbeats.h"

#define TRACKBEATS_INDEX_RANGE 20


TrackBeats::TrackBeats(TrackPointer tio) : m_qMutex(QMutex::Recursive)
{
    m_iSampleRate = tio->getSampleRate();
}

TrackBeats::~TrackBeats()
{
}

/**
 * Private function that returns the sample index. This is an index into 
 * m_beatIndex which holds the first sample offset that holds a beat for a 
 * certain range of sample offsets.
 */
int TrackBeats::sampleIndex(int sample) const
{
    QMutexLocker lock(&m_qMutex);
    return (int) round(sample / (m_iSampleRate * TRACKBEATS_INDEX_RANGE));
}

/**
 * Get the count of beats stored.
 */
int TrackBeats::getBeatCount() const
{
    QMutexLocker lock(&m_qMutex);
    return m_beats.size();
}

/**
 * Add a beat at 'sample' sample.
 */
void TrackBeats::addBeatSample(int sample)
{
    QMutexLocker lock(&m_qMutex);
    int index = sampleIndex(sample);


    if ((m_beatIndex.size()-1) < index ) 
    {
        int i;
        
        
        for (i = m_beatIndex.size() - 1; i < index; i++ )
            m_beatIndex.append(sample);
        
        m_beatIndex.append(sample);
    }
    else
    {
        if ( m_beatIndex.at(index) > sample )
            m_beatIndex[index] = sample;
    }

    m_beats[sample] = sample;
}

/**
 * Remove the beat with the sample offset specified.
 */
void TrackBeats::removeBeatSample(int sample)
{
    QMutexLocker lock(&m_qMutex);
    int index = sampleIndex(sample);
    int d;
    int newSample = -1;

    m_beats.remove(sample);

    if ( ! m_beatIndex.contains(sample))
        return;

    if ((index+1) < m_beatIndex.size()) {
        newSample = m_beatIndex.at(index+1);
        
        for (d = index; d > 0 && m_beatIndex.at(d) >= sample; d--) {
            if ( m_beatIndex.at(d) == sample )
                m_beatIndex[d] = newSample;
        }
    }
    else {
        for (d = index; d > 0 && m_beatIndex.at(d) >= sample; d--) {
            if ( m_beatIndex.at(d) == sample )
                m_beatIndex.removeAt(d);
        }
    }

    if ( m_beatIndex.contains(sample) )
        qDebug() << "Dangling Indexes:" << m_beatIndex.count(sample);
}

/**
 * Add a beat at 'beat' seconds.
 */
void TrackBeats::addBeatSeconds(double beat)
{
    QMutexLocker lock(&m_qMutex);
    addBeatSample((int)round(beat * m_iSampleRate));
}


/**
 * Remove a beat at 'beat' seconds.
 */
void TrackBeats::removeBeatSeconds(double beat)
{
    QMutexLocker lock(&m_qMutex);
    removeBeatSample((int)round(beat * m_iSampleRate));
}

/**
 * Find the Next beat starting from offset sample.
 */
int TrackBeats::findNextBeatSample(int sample) const
{
    QMutexLocker lock(&m_qMutex);
    QMapIterator<int, int> iter(m_beats);
    int index = sampleIndex(sample);
    
    
    if (m_beatIndex.size() > index)
    {
        iter.findNext(m_beatIndex.value(index));
        do {
            iter.next();
        } while((iter.hasNext()) && (iter.value() <= sample));
        
        return iter.value();
    }
    
    return -1;
}

/**
 * Find the Previous beat starting from offset sample.
 */
int TrackBeats::findPrevBeatSample(int sample) const
{
    QMutexLocker lock(&m_qMutex);
    QMapIterator<int, int> iter(m_beats);
    int index = sampleIndex(sample);
    
    if (m_beatIndex.size() > index)
    {
        iter.findNext(m_beatIndex.value(index));
        do {
            iter.previous();
        } while((iter.hasPrevious()) && (iter.value() >= sample));
        
        return iter.value();
    }
    
    return -1;
}

/**
 * Find the Nth beat (offset) from sample .
 */
int TrackBeats::findBeatOffsetSamples(int sample, int offset) const
{
    QMutexLocker lock(&m_qMutex);
    QMapIterator<int, int> iter(m_beats);
    int index = sampleIndex(sample);
    int i;
    
    
    if (m_beatIndex.size() < index)
        return -1;
    
    
    iter.findNext(m_beatIndex.value(index));
    do {
        iter.next();
    } while((iter.hasNext()) && (iter.value() <= sample));
    
    // Backup one just to be before the marker
    if ((iter.hasPrevious()) && (iter.value() > sample))
        iter.previous();
    
    // Find the offset from the current beat
    if ( offset > 0 )
    {
        for (i = 0; i < offset && iter.hasNext(); i++)
            iter.next();
    }
    else if ( offset < 0 )
    {
        for (i = offset * -1; i > 0 && iter.hasPrevious(); i--)
            iter.previous();
    }
    
    return iter.value();
}

/**
 * Return TRUE if there is any beats between start and stop (in samples)
 */
bool TrackBeats::hasBeatsSamples(double start, double stop) const
{
    QMutexLocker lock(&m_qMutex);
    QMapIterator<int, int> iter(m_beats);
    int index = sampleIndex(start);


    if (iter.findNext(m_beatIndex.value(index)))
    {
        do {
            if ((iter.value() >= start) && (iter.value() <= stop))
                 return true;
            
            iter.next();
        } while((iter.hasNext()) && (iter.value() <= stop));
    }
    
    return false;
}

/**
 * Find and return a list of all beats between sample offset start and stop.
 */
QList<int>* TrackBeats::findBeatsSamples(int start, int stop) const
{
    QMutexLocker lock(&m_qMutex);
    QList<int> *ret = new QList<int>;
    QMapIterator<int, int> iter(m_beats);
    int index = sampleIndex(start);
    
    
    if (iter.findNext(m_beatIndex.value(index)))
    {
        do {
            if ((iter.value() >= start) && (iter.value() <= stop))
                 ret->append(iter.value());
            
            iter.next();
        } while((iter.hasNext()) && (iter.value() <= stop));
    }
    
    return ret;
}

/**
 * Find the Next beat starting from an offset in seconds.
 */
double TrackBeats::findNextBeatSeconds(double beat) const
{
    QMutexLocker lock(&m_qMutex);
    int sample = (int) round(beat * m_iSampleRate);
    return findNextBeatSample(sample) / m_iSampleRate;
}

/**
 * Find the Previous beat starting from an offset in seconds.
 */
double TrackBeats::findPrevBeatSeconds(double beat) const
{
    QMutexLocker lock(&m_qMutex);
    int sample = (int) round(beat * m_iSampleRate);
    return findPrevBeatSample(sample) / m_iSampleRate;
}

/**
 * Find the Nth beat (offset) from offset seconds in seconds.
 */
double TrackBeats::findBeatOffsetSeconds(double seconds, int offset) const
{
    QMutexLocker lock(&m_qMutex);
    int bgn = round(seconds * m_iSampleRate);
    
    return ((double)findBeatOffsetSamples(bgn, offset) / m_iSampleRate);
}

/**
 * Return TRUE if there is any beats between start and stop (in seconds)
 */
bool TrackBeats::hasBeatsSeconds(double start, double stop) const
{
    QMutexLocker lock(&m_qMutex);
    int bgn = round(start * m_iSampleRate);
    int end = round(stop * m_iSampleRate);

    return hasBeatsSamples(bgn, end);
}

/**
 * Find and return a list of all beats between seconds start and stop.
 */
QList<double>* TrackBeats::findBeatsSeconds(double start, double stop) const
{
    QMutexLocker lock(&m_qMutex);
    QList<double> *ret = new QList<double>;
    QList<int>* samples;
    int begin = round(start * m_iSampleRate);
    int end = round(stop * m_iSampleRate);
    int i;
    
    
    samples = findBeatsSamples(begin, end);
    for (i = 0; i < samples->size(); i++)
    {
        ret->append((double)samples->at(i) / (double)m_iSampleRate);
    }
    
    delete samples;
    return ret;
}

QByteArray *TrackBeats::serializeToBlob()
{
    QMutexLocker lock(&m_qMutex);
    QByteArray *blob;
    int *buffer = new int[getBeatCount()];
    int *ptr = buffer;
    QMapIterator<int, int> iter(m_beats);
    
    
    iter.next();
    
    while ( iter.hasNext())
    {
        *ptr++ = iter.value();
        iter.next();
    }
    
    blob = new QByteArray((char *)buffer, getBeatCount() * sizeof(int));
    delete []buffer;

    return blob;
}

void TrackBeats::unserializeFromBlob(QByteArray *blob)
{
    QMutexLocker lock(&m_qMutex);
    int *ptr = (int *)blob->constData();
    int i;
    
    
    for (i = blob->size() / sizeof(int); --i; ptr++)
    {
        addBeatSample(*ptr);
    }
}

