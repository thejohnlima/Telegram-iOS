import Foundation
import Postbox
import TelegramCore

func stringForTimestamp(day: Int32, month: Int32, year: Int32, dateTimeFormat: PresentationDateTimeFormat) -> String {
    let separator = dateTimeFormat.dateSeparator
    switch dateTimeFormat.dateFormat {
        case .monthFirst:
            return String(format: "%d%@%d%@%02d", month, separator, day, separator, year - 100)
        case .dayFirst:
            return String(format: "%d%@%02d%@%02d", day, separator, month, separator, year - 100)
    }
}

func stringForTimestamp(day: Int32, month: Int32, dateTimeFormat: PresentationDateTimeFormat) -> String {
    let separator = dateTimeFormat.dateSeparator
    switch dateTimeFormat.dateFormat {
    case .monthFirst:
        return String(format: "%d%@%d", month, separator, day)
    case .dayFirst:
        return String(format: "%d%@%02d", day, separator, month)
    }
}

func shortStringForDayOfWeek(strings: PresentationStrings, day: Int32) -> String {
    switch day {
        case 0:
            return strings.Weekday_ShortSunday
        case 1:
            return strings.Weekday_ShortMonday
        case 2:
            return strings.Weekday_ShortTuesday
        case 3:
            return strings.Weekday_ShortWednesday
        case 4:
            return strings.Weekday_ShortThursday
        case 5:
            return strings.Weekday_ShortFriday
        case 6:
            return strings.Weekday_ShortSaturday
        default:
            return ""
    }
}

func stringForMonth(strings: PresentationStrings, month: Int32) -> String {
    switch month {
        case 0:
            return strings.Month_GenJanuary
        case 1:
            return strings.Month_GenFebruary
        case 2:
            return strings.Month_GenMarch
        case 3:
            return strings.Month_GenApril
        case 4:
            return strings.Month_GenMay
        case 5:
            return strings.Month_GenJune
        case 6:
            return strings.Month_GenJuly
        case 7:
            return strings.Month_GenAugust
        case 8:
            return strings.Month_GenSeptember
        case 9:
            return strings.Month_GenOctober
        case 10:
            return strings.Month_GenNovember
        case 11:
            return strings.Month_GenDecember
        default:
            return ""
    }
}

func stringForMonth(strings: PresentationStrings, month: Int32, ofYear year: Int32) -> String {
    return stringForMonth(strings: strings, month: month) + " \(1900 + year)"
}

enum RelativeTimestampFormatDay {
    case today
    case yesterday
}

func stringForUserPresence(strings: PresentationStrings, day: RelativeTimestampFormatDay, dateTimeFormat: PresentationDateTimeFormat, hours: Int32, minutes: Int32) -> String {
    let dayString: String
    switch day {
        case .today:
            dayString = strings.LastSeen_AtDate(strings.Time_TodayAt(stringForShortTimestamp(hours: hours, minutes: minutes, dateTimeFormat: dateTimeFormat)).0).0
        case .yesterday:
            dayString = strings.LastSeen_YesterdayAt(stringForShortTimestamp(hours: hours, minutes: minutes, dateTimeFormat: dateTimeFormat)).0
    }
    return dayString
}

private func humanReadableStringForTimestamp(strings: PresentationStrings, day: RelativeTimestampFormatDay, dateTimeFormat: PresentationDateTimeFormat, hours: Int32, minutes: Int32) -> String {
    let dayString: String
    switch day {
    case .today:
        dayString = strings.Time_TodayAt(stringForShortTimestamp(hours: hours, minutes: minutes, dateTimeFormat: dateTimeFormat)).0
    case .yesterday:
        dayString = strings.Time_YesterdayAt(stringForShortTimestamp(hours: hours, minutes: minutes, dateTimeFormat: dateTimeFormat)).0
    }
    return dayString
}

func humanReadableStringForTimestamp(strings: PresentationStrings, dateTimeFormat: PresentationDateTimeFormat, timestamp: Int32) -> String {
    var t: time_t = time_t(timestamp)
    var timeinfo: tm = tm()
    localtime_r(&t, &timeinfo)
    
    let timestampNow = Int32(CFAbsoluteTimeGetCurrent() + NSTimeIntervalSince1970)
    var now: time_t = time_t(timestampNow)
    var timeinfoNow: tm = tm()
    localtime_r(&now, &timeinfoNow)
    
    if timeinfo.tm_year != timeinfoNow.tm_year {
        return "\(stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat))"
    }
    
    let dayDifference = timeinfo.tm_yday - timeinfoNow.tm_yday
    if dayDifference == 0 || dayDifference == -1 {
        let day: RelativeTimestampFormatDay
        if dayDifference == 0 {
            day = .today
        } else {
            day = .yesterday
        }
        return humanReadableStringForTimestamp(strings: strings, day: day, dateTimeFormat: dateTimeFormat, hours: timeinfo.tm_hour, minutes: timeinfo.tm_min)
    } else {
        return "\(stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat))"
    }
}

enum RelativeUserPresenceLastSeen {
    case justNow
    case minutesAgo(Int32)
    case hoursAgo(Int32)
    case todayAt(hours: Int32, minutes: Int32)
    case yesterdayAt(hours: Int32, minutes: Int32)
    case thisYear(month: Int32, day: Int32)
    case atDate(year: Int32, month: Int32)
}

enum RelativeUserPresenceStatus {
    case offline
    case online(at: Int32)
    case lastSeen(at: Int32)
    case recently
    case lastWeek
    case lastMonth
}

func relativeUserPresenceStatus(_ presence: TelegramUserPresence, relativeTo timestamp: Int32) -> RelativeUserPresenceStatus {
    switch presence.status {
        case .none:
            return .offline
        case let .present(statusTimestamp):
            if statusTimestamp >= timestamp {
                return .online(at: statusTimestamp)
            } else {
                return .lastSeen(at: statusTimestamp)
            }
        case .recently:
            let activeUntil = presence.lastActivity + 30
            if activeUntil >= timestamp {
                return .online(at: activeUntil)
            } else {
                return .recently
            }
        case .lastWeek:
            return .lastWeek
        case .lastMonth:
            return .lastMonth
    }
}

func stringForRelativeTimestamp(strings: PresentationStrings, relativeTimestamp: Int32, relativeTo timestamp: Int32, dateTimeFormat: PresentationDateTimeFormat) -> String {
    var t: time_t = time_t(relativeTimestamp)
    var timeinfo: tm = tm()
    localtime_r(&t, &timeinfo)
    
    var now: time_t = time_t(timestamp)
    var timeinfoNow: tm = tm()
    localtime_r(&now, &timeinfoNow)
    
    if timeinfo.tm_year != timeinfoNow.tm_year {
        return stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat)
    }
    
    let dayDifference = timeinfo.tm_yday - timeinfoNow.tm_yday
    if dayDifference > -7 {
        if dayDifference == 0 {
            return stringForShortTimestamp(hours: timeinfo.tm_hour, minutes: timeinfo.tm_min, dateTimeFormat: dateTimeFormat)
        } else {
            return shortStringForDayOfWeek(strings: strings, day: timeinfo.tm_wday)
        }
    } else {
        return stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, dateTimeFormat: dateTimeFormat)
    }
}

func stringForRelativeLiveLocationTimestamp(strings: PresentationStrings, relativeTimestamp: Int32, relativeTo timestamp: Int32, dateTimeFormat: PresentationDateTimeFormat) -> String {
    let difference = timestamp - relativeTimestamp
    if difference < 60 {
        return strings.LiveLocationUpdated_JustNow
    } else if difference < 60 * 60 {
        let minutes = difference / 60
        return strings.LiveLocationUpdated_MinutesAgo(minutes)
    } else {
        var t: time_t = time_t(relativeTimestamp)
        var timeinfo: tm = tm()
        localtime_r(&t, &timeinfo)
        
        var now: time_t = time_t(timestamp)
        var timeinfoNow: tm = tm()
        localtime_r(&now, &timeinfoNow)
        
        let dayDifference = timeinfo.tm_yday - timeinfoNow.tm_yday
        
        let hours = timeinfo.tm_hour
        let minutes = timeinfo.tm_min
        
        if dayDifference == 0 {
            return strings.LiveLocationUpdated_TodayAt(stringForShortTimestamp(hours: hours, minutes: minutes, dateTimeFormat: dateTimeFormat)).0
        } else {
            return stringForFullDate(timestamp: relativeTimestamp, strings: strings, dateTimeFormat: dateTimeFormat)
        }
    }
}

func stringForRelativeLiveLocationUpdateTimestamp(strings: PresentationStrings, relativeTimestamp: Int32, relativeTo timestamp: Int32, dateTimeFormat: PresentationDateTimeFormat) -> String {
    var t: time_t = time_t(relativeTimestamp)
    var timeinfo: tm = tm()
    localtime_r(&t, &timeinfo)
    
    var now: time_t = time_t(timestamp)
    var timeinfoNow: tm = tm()
    localtime_r(&now, &timeinfoNow)
    
    if timeinfo.tm_year != timeinfoNow.tm_year {
        return stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat)
    }
    
    let dayDifference = timeinfo.tm_yday - timeinfoNow.tm_yday
    if dayDifference > -7 {
        if dayDifference == 0 {
            return stringForShortTimestamp(hours: timeinfo.tm_hour, minutes: timeinfo.tm_min, dateTimeFormat: dateTimeFormat)
        } else {
            return shortStringForDayOfWeek(strings: strings, day: timeinfo.tm_wday)
        }
    } else {
        return stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, dateTimeFormat: dateTimeFormat)
    }
}

func stringAndActivityForUserPresence(strings: PresentationStrings, dateTimeFormat: PresentationDateTimeFormat, presence: TelegramUserPresence, relativeTo timestamp: Int32) -> (String, Bool) {
    switch presence.status {
        case .none:
            return (strings.LastSeen_Offline, false)
        case let .present(statusTimestamp):
            if statusTimestamp >= timestamp {
                return (strings.Presence_online, true)
            } else {
                let difference = timestamp - statusTimestamp
                if difference < 60 {
                    return (strings.LastSeen_JustNow, false)
                } else if difference < 60 * 60 {
                    let minutes = difference / 60
                    return (strings.LastSeen_MinutesAgo(minutes), false)
                } else {
                    var t: time_t = time_t(statusTimestamp)
                    var timeinfo: tm = tm()
                    localtime_r(&t, &timeinfo)
                    
                    var now: time_t = time_t(timestamp)
                    var timeinfoNow: tm = tm()
                    localtime_r(&now, &timeinfoNow)
                    
                    if timeinfo.tm_year != timeinfoNow.tm_year {
                        return (strings.LastSeen_AtDate(stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat)).0, false)
                    }
                    
                    let dayDifference = timeinfo.tm_yday - timeinfoNow.tm_yday
                    if dayDifference == 0 || dayDifference == -1 {
                        let day: RelativeTimestampFormatDay
                        if dayDifference == 0 {
                            let minutes = difference / (60 * 60)
                            return (strings.LastSeen_HoursAgo(minutes), false)
                        } else {
                            day = .yesterday
                        }
                        return (stringForUserPresence(strings: strings, day: day, dateTimeFormat: dateTimeFormat, hours: timeinfo.tm_hour, minutes: timeinfo.tm_min), false)
                    } else {
                        return (strings.LastSeen_AtDate(stringForTimestamp(day: timeinfo.tm_mday, month: timeinfo.tm_mon + 1, year: timeinfo.tm_year, dateTimeFormat: dateTimeFormat)).0, false)
                    }
                }
            }
        case .recently:
            let activeUntil = presence.lastActivity + 30
            if activeUntil >= timestamp {
                return (strings.Presence_online, true)
            } else {
                return (strings.LastSeen_Lately, false)
            }
        case .lastWeek:
            return (strings.LastSeen_WithinAWeek, false)
        case .lastMonth:
            return (strings.LastSeen_WithinAMonth, false)
    }
}

func userPresenceStringRefreshTimeout(_ presence: TelegramUserPresence, relativeTo timestamp: Int32) -> Double {
    switch presence.status {
        case let .present(statusTimestamp):
            if statusTimestamp >= timestamp {
                return Double(statusTimestamp - timestamp)
            } else {
                let difference = timestamp - statusTimestamp
                if difference < 30 {
                    return Double((30 - difference) + 1)
                } else if difference < 60 * 60 {
                    return Double((difference % 60) + 1)
                } else {
                    return Double.infinity
                }
            }
        case .recently:
            let activeUntil = presence.lastActivity + 30
            if activeUntil >= timestamp {
                return Double(activeUntil - timestamp + 1)
            } else {
                return Double.infinity
            }
        case .none, .lastWeek, .lastMonth:
            return Double.infinity
    }
}


func stringForRemainingMuteInterval(strings: PresentationStrings, muteInterval value: Int32) -> String {
    let timestamp = Int32(CFAbsoluteTimeGetCurrent() + NSTimeIntervalSince1970)
    let value = max(1 * 60, value - timestamp)
    if value <= 1 * 60 * 60 {
        return strings.MuteExpires_Minutes(Int32(round(Float(value) / 60)))
    } else if value <= 24 * 60 * 60 {
        return strings.MuteExpires_Hours(Int32(round(Float(value) / (60 * 60))))
    } else {
        return strings.MuteExpires_Days(Int32(round(Float(value) / (24 * 60 * 60))))
    }
}
