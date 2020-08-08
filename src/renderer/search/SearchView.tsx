import React, { useState } from 'react'
import styled from 'styled-components'
import { TrackVolume } from './TrackVolume'
import { send } from '../bitwig-api/Bitwig'

const Result = styled.div`
    user-select: none;
    background: ${props => props.selected ? `#888` : `#444`};
    padding: .3em 1.3em;
    font-size: 1.1em;
    border-bottom: 1px solid #111;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding-left: 2.1em;
    > * {
        flex-shrink: 0;
    }
    position: relative;
`
const FlexGrow = styled.div`
    flex-grow: 1;
`
const Color = styled.div`
    background: ${props => props.color};
    position: absolute;
    top: 0;
    bottom: 0;
    left: 0;
    width: 1em;  
`
const Recent = styled.div`
    font-size: .8em;
    margin-right: 1em;
`

const Input = styled.input`
    width: 100%;
    background: #222;
    outline: none !important;
    color: white;
    box-shadow: none;
    border: none;
    font-size: 1.2em;
    padding: 1em;
`

const FlexContainer = styled.div`
    position: absolute;
    top: 0;
    bottom: 0;
    left: 0;
    right: 0;
    display: flex;
    flex-direction: column;
    > input {
        flex-grow: 0;
        flex-shrink: 0;
    }
`
const FlexScroll = styled.div`
    overflow: auto;
    flex-grow: 1;
    flex-shrink: 1;
`

const MuteSolo = styled.div`
    width: 1.8em;
    height: 1.5em;
    display: flex;
    align-items: center;
    border: 1px solid black;
    justify-content: center;
    background: ${props => props.active ? props.activeColor : '#666'};
    color: ${props => props.active ? 'black' : 'white'};
    margin-right: .6em;
    border-radius: 0.3em;
    cursor: pointer;
    font-weight: 900;
`
type TrackResultProps = {
    result: SearchResult, 
    onConfirmed: (result: SearchResult) => void, 
    onShouldSelect: (result: SearchResult) => void
}
const TrackResult = ({result, onConfirmed, onShouldSelect}: TrackResultProps) => {
    const [ solo, setSolo] = useState(result.track.solo)
    const [ mute, setMute] = useState(result.track.mute)
    const toggleSolo = () => {
        send({
            type: 'track/update',
            data: {
                name: result.track.name,
                solo: !solo
            }
        })
        setSolo(!solo)
    }
    const toggleMute = () => {
        send({
            type: 'track/update',
            data: {
                name: result.track.name,
                mute: !mute
            }
        })
        setMute(!mute)
    }
    const onDoubleClick = event => {
        event.stopPropagation()
    }
    return <Result id={result.selected ? 'selectedtrack' : ''} key={result.id} selected={result.selected} onDoubleClick={() => onConfirmed(result)} onMouseDown={() => onShouldSelect(result)}>
        <Color color={result.color} /> 
        <span>{result.title}</span> 
        <FlexGrow /> 
        {result.isRecent ? <Recent>⭐</Recent> : null}
        <MuteSolo onDoubleClick={onDoubleClick} activeColor={`#D0C609`} active={solo} onClick={toggleSolo}>S</MuteSolo>
        <MuteSolo onDoubleClick={onDoubleClick} activeColor={`#F97012`} active={mute} onClick={toggleMute}>M</MuteSolo>
        <TrackVolume track={result.track} />
    </Result>
}

export interface SearchResult {
    title: string
    id: string

    track: any

    isRecent?: boolean
    color: string

    icon?: React.ElementType

    selected: boolean
}

export interface SearchProps {
    onQueryChanged: (query: string) => void,
    onCancelled: () => void,
    results: SearchResult[],
    query: string,
    onShouldSelect: (result: SearchResult) => void,
    onConfirmed: (result: SearchResult) => void,
    placeholder: string
}

export class SearchView extends React.Component<SearchProps> {
    repeatInterval: any
    onInputChange = event => {
        this.props.onQueryChanged(event.target.value)
    }
    onKey = (keyCode: number) => {
        const selectedIndex = this.props.results.findIndex(r => r.selected)
        if (selectedIndex < 0) {
            return
        }
        if (keyCode === 38) {
            // up
            const newSelect = this.props.results[Math.max(0, selectedIndex - 1)]
            this.props.onShouldSelect(newSelect)
            this.waitingForScroll = true
        } else if (keyCode === 40) {
            // down
            const newSelect = this.props.results[Math.min(this.props.results.length - 1, selectedIndex + 1)]
            this.props.onShouldSelect(newSelect)
            this.waitingForScroll = true
        }
    }
    waitingForScroll = false
    getSelected = () => {
        return this.props.results.find(r => r.selected)
    }
    onKeyDown = (event) => {
        if (event.key === ' ' && this.props.query === '') {
            event.preventDefault()
            send({
                type: 'transport/play'
            })
            return
        }

        clearInterval(this.repeatInterval)
        if (event.keyCode === 27) {
            // escape
            return this.props.onCancelled()
        } else if (event.keyCode === 13) {
            // enter
            const selected = this.getSelected()
            return this.props.onConfirmed(selected)
        }
        const keyCode = event.keyCode
        this.repeatInterval = setInterval(() => {
            this.onKey(keyCode)
        }, 100)
        this.onKey(keyCode)
    }
    onKeyUp = () => {
        clearInterval(this.repeatInterval)
    }
    componentDidMount() {
        window.addEventListener('keydown', this.onKeyDown)
        window.addEventListener('keyup', this.onKeyUp)
    }
    componentWillUnmount() {
        window.removeEventListener('keydown', this.onKeyDown)
        window.removeEventListener('keyup', this.onKeyUp)
    }
    componentDidUpdate(prevProps) {
        if (this.waitingForScroll) {
            document.getElementById('selectedtrack')?.scrollIntoView({
                block: 'end'
            })
            this.waitingForScroll = false
        }
        document.getElementById('theinput').focus()
        setTimeout(() => {
            document.getElementById('theinput').focus()
        }, 100)
    }
    onSearchKeyDown = event => {
        // Don't allow up/down arrow keys to navigate input
        if (event.keyCode === 38 || event.keyCode === 40) {
            event.preventDefault()
        }
    }
    render() {
        return <FlexContainer style={{fontSize: '.9rem'}}>
            <Input id="theinput" autoComplete={"off"} autoCorrect={"off"} autoCapitalize={"off"} spellCheck={false} autoFocus onKeyDown={this.onSearchKeyDown} placeholder={this.props.placeholder} 
            onChange={this.onInputChange} value={this.props.query} />
            <FlexScroll>
                {this.props.results.map(result => <TrackResult key={result.id} result={result} onConfirmed={this.props.onConfirmed} onShouldSelect={this.props.onShouldSelect} />)}
            </FlexScroll>
        </FlexContainer>
    }
}